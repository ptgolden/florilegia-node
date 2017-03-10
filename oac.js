"use strict";

const fs = require('fs')
    , path = require('path')
    , glob = require('glob')
    , { Writer, Util } = require('n3')
    , { createLiteral } = Util
    , { getAnnotations } = require('./')

const prefixes = {
  oa: require('lov-ns/oa'),
  dce: require('lov-ns/dce'),
  dctype: require('lov-ns/dctype'),
  cnt: require('lov-ns/cnt'),
  bibo: require('lov-ns/bibo'),
  rdf: require('lov-ns/rdf'),
  rdfs: require('lov-ns/rdfs'),
}

const $ = require('rdf-builder')({ prefixes })

function makeURITag(email, filename) {
  const d = new Date()

  return [
    'tag',
    email,
    `${d.getFullYear()}-${d.getMonth()}-${d.getDate()}`,
    path.basename(filename)
  ].join(':')
}

function pdf2oac(user='http://example.com', filename, cb) {
  const writer = new Writer({ prefixes, format: 'ntriples' })
      , annotations = getAnnotations(filename)

  let triples = []

  annotations.forEach((annotation, i) => {
    const { page, motivation, body_text, highlighted_text } = annotation
        , baseTag = makeURITag(user, filename, `annot-${i + 1}`)
        , makeAnnotURI = p => $(`${baseTag}/${p ? p : ''}`)

    const $annot = makeAnnotURI()
        , $target = makeAnnotURI('target')
        , $pdfSelector = makeAnnotURI('page-selector')

    triples = triples.concat(
      $annot({
        'rdf:type': 'oa:Annotation',
        'oa:hasMotivation': `oa:${motivation}`,
        'oa:hasTarget': $target,
        'oa:hasSource': `file://${filename}`,
      }),

      $target('oa:hasSelector')($pdfSelector),

      $pdfSelector({
        'rdf:type': 'oa:FragmentSelector',
        'rdf:value': createLiteral(`#page=${page}`),
        'oa:conformsTo': 'http://tools.ietf.org/rfc/rfc3778',
      })
    )

    // Text selector
    if (highlighted_text) {
      const $textSelector = makeAnnotURI('text-selector')

      triples = triples.concat(
        $target('oa:hasSelector')($textSelector),

        $textSelector({
          'rdf:type': 'oa:TextQuoteSelector',
          'oa:exact': createLiteral(highlighted_text),
        })
      )
    }

    // Text body (i.e. comment)
    if (body_text) {
      const $commentBody = makeAnnotURI('comment-body')

      triples = triples.concat(
        $annot('oa:hasBody')($commentBody),

        $commentBody({
          'dce:format': createLiteral('text/plain'),
          'rdf:type': ['dctype:Text', 'cnt:ContentAsText'],
          'cnt:chars': createLiteral(body_text),
        })
      )
    }

    if (motivation === 'bookmarking') {
      const file = glob.sync('TEST/' + annotation.object_id + '*').slice(-1)[0];

      if (file) {
        const $imageBody = makeAnnotURI('stamp')
            , content = fs.readFileSync(file)
            , buf = new Buffer(content);

        triples = triples.concat(
          $annot('oa:hasBody')($imageBody),

          $imageBody({
            'dce:format': createLiteral('image/png'),
            'rdf:type': ['dctype:Image', 'cnt:ContentAsBase64'],
            'cnt:bytes': createLiteral(buf.toString('base64')),
          })
        )
      }

    }

    triples.forEach(triple => writer.addTriple(triple));
  });

  writer.end(cb);
}

module.exports = { pdf2oac }
