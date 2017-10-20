"use strict";

const through = require('through2')
    , rdfBuilder = require('rdf-builder')
    , { createLiteral } = require('n3').Util
    , prefixes = require('./prefixes')

module.exports = { annotTransform, annotToTriples }

function fnState(opts) {
  return {
    i: 0,
    images: {},
    $: opts.graphURI
      ? rdfBuilder({ prefixes, graph: opts.graphURI })
      : rdfBuilder({ prefixes })
  }
}

function isNumeric(string) {
  return /^\d+$/.test(string)
}

function annotTransform(opts) {
  const state = fnState(opts)

  return through.obj(function (annotation, enc, cb) {
    annotToTriples(annotation, opts, state).forEach(triple => {
      this.push(triple);
    });

    state.i += 1

    cb();
  });
}

function defaultAnnotURI(part, i, annot,opts) {
  return `${opts.baseURI || ''}annotation-${i}${part ? ('/' + part) : ''}`
}

function annotToTriples(annotation, opts={}, state=fnState(opts)) {
  const {
    pdfURI='ex:pdf',
    baseURI='http://example.org/#',
    mintAnnotURI=defaultAnnotURI
  } = opts

  const { $, i, images } = state

  let triples = []

  const {
    page,
    motivation,
    body_text,
    body_modified,
    body_creator,
    target_text
  } = annotation

  const makeAnnotURI = p => $(mintAnnotURI(p, i, annotation, opts))

  const $annot = makeAnnotURI()
      , $target = makeAnnotURI('target')
      , $pdfSelector = makeAnnotURI('page-selector')

  triples = triples.concat(
    $annot({
      'rdf:type': 'oa:Annotation',
      'oa:hasMotivation': `oa:${motivation}`,
      'oa:hasTarget': $target,
    }),


    $target({
      'oa:hasSource': pdfURI,
      'oa:hasSelector': $pdfSelector
    }),

    $pdfSelector({
      'rdf:type': 'oa:FragmentSelector',
      'rdf:value': createLiteral(`#page=${page}&highlight=${annotation.target_rect}`),
      'oa:conformsTo': 'http://tools.ietf.org/rfc/rfc3778',
    })
  )

  if (body_modified && isNumeric(body_modified)) {
    triples.push(
      $annot('dc:modified')(createLiteral(
        new Date(parseInt(body_modified)).toISOString(),
        prefixes.xsd + 'date'
      ))
    )
  }

  if (body_creator) {
    triples.push($annot('dc:creator')(createLiteral(body_creator)));
  }


  // Text selector
  if (target_text) {
    const $textSelector = makeAnnotURI('text-selector')

    triples = triples.concat(
      $target('oa:hasSelector')($textSelector),

      $textSelector({
        'rdf:type': 'oa:TextQuoteSelector',
        'oa:exact': createLiteral(target_text),
      })
    )
  }

  // Text body (i.e. comment)
  if (body_text) {
    const $commentBody = makeAnnotURI('comment-body')

    triples = triples.concat(
      $annot('oa:hasBody')($commentBody),

      $commentBody({
        'dc:format': createLiteral('text/plain'),
        'rdf:type': ['dctype:Text', 'cnt:ContentAsText'],
        'cnt:chars': createLiteral(body_text),
      })
    )
  }

  if (motivation === 'bookmarking' && annotation.stamp_body) {
    const $imageURI = $(`${baseURI}image-${Object.keys(images).length}`)

    triples = triples.concat(
      $imageURI({
        'dc:identifier': createLiteral(annotation.body_subject),
        'dc:format': createLiteral('image/png'),
        'rdf:type': ['dctype:Image', 'cnt:ContentAsBase64'],
        'cnt:bytes': createLiteral(annotation.stamp_body),
      })
    )

    images[annotation.stamp_body] = $imageURI;

    triples = triples.concat(
      $annot('oa:hasBody')(images[annotation.stamp_body])
    )
  }
  return triples
}
