"use strict";

const fs = require('fs')
    , path = require('path')
    , glob = require('glob')
    , { Writer, Util } = require('n3')
    , { getAnnotations } = require('./')

const prefixes = {
  oa: "http://www.w3.org/ns/oa#",
  dc: "http://purl.org/dc/elements/1.1/",
  dctypes: "http://purl.org/dc/dcmitype/",
  cnt: "http://www.w3.org/2011/content#",
  bibo: "http://purl.org/ontology/bibo/",
  rdf: "http://www.w3.org/1999/02/22-rdf-syntax-ns#",
  rdfs: "http://www.w3.org/2000/01/rdf-schema#",
}

const $ = require('rdf-triple-builder')({ prefixes })

function makeTag(email, filename) {
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
        , $annot = $(makeTag(user, filename, `annot-${i + 1}`))
        , makeURI = p => $(`${$annot}/${p}`)
        , $target = makeURI('target')
        , $pdfSelector = makeURI('page-selector')

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
        'rdf:value': Util.createLiteral(`#page=${page}`),
        'oa:conformsTo': 'http://tools.ietf.org/rfc/rfc3778',
      })
    )

    // Text selector
    if (highlighted_text) {
      const $textSelector = makeURI('text-selector')

      triples = triples.concat(
        $target('oa:hasSelector')($textSelector),

        $textSelector({
          'rdf:type': 'oa:TextQuoteSelector',
          'oa:exact': Util.createLiteral(highlighted_text),
        })
      )
    }

    // Text body (i.e. comment)
    if (body_text) {
      const $commentBody = makeURI('comment-body')

      triples = triples.concat(
        $annot('oa:hasBody')($commentBody),

        $commentBody({
          'dc:format': Util.createLiteral('text/plain'),
          'rdf:type': ['dctypes:Text', 'cnt:ContentAsText'],
          'cnt:chars': Util.createLiteral(body_text),
        })
      )
    }

    if (motivation === 'bookmarking') {
      const file = glob.sync('TEST/' + annotation.object_id + '*').slice(-1)[0];

      if (file) {
        const $imageBody = makeURI('stamp')
            , content = fs.readFileSync(file)
            , buf = new Buffer(content);

        triples = triples.concat(
          $annot('oa:hasBody')($imageBody),

          $imageBody({
            'dc:format': Util.createLiteral('image/png'),
            'rdf:type': ['dctypes:Image', 'cnt:ContentAsBase64'],
            'cnt:bytes': Util.createLiteral(buf.toString('base64')),
          })
        )
      }

    }

    triples.forEach(triple => writer.addTriple(triple));
  });

  writer.end(cb);
}

module.exports = { pdf2oac }
