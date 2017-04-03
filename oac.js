"use strict";

const fs = require('fs')
    , glob = require('glob')
    , { Util, Writer } = require('n3')
    , { createLiteral } = Util

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

module.exports = function annotationsToOAC(annotations, opts={}) {
  const {
    imageDirectory,
    pdfURI,
    baseURI='http://example.org/#',
    outstream=process.stdout
  } = opts

  const writer = new Writer(outstream, {
    prefixes,
    format: 'ntriples',
    end: outstream !== process.stdout
  })

  const images = {}

  annotations.forEach((annotation, i) => {
    let triples = []

    const { page, motivation, body_text, highlighted_text } = annotation
        , makeAnnotURI = p => $(`${baseURI}annotation-${i}/${p ? p : ''}`)

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

    if (motivation === 'bookmarking' && imageDirectory) {
      const file = glob.sync(imageDirectory + '/' + annotation.object_id + '*').slice(-1)[0];

      if (file) {
        const content = fs.readFileSync(file)
            , buf = new Buffer(content)
            , b64 = buf.toString('base64')

        if (!images.hasOwnProperty(b64)) {
          const $imageURI = $(`${baseURI}image-${Object.keys(images).length}`)

          triples = triples.concat(
            $imageURI({
              'dce:format': createLiteral('image/png'),
              'rdf:type': ['dctype:Image', 'cnt:ContentAsBase64'],
              'cnt:bytes': createLiteral(buf.toString('base64')),
            })
          )

          images[b64] = $imageURI;
        }

        triples = triples.concat(
          $annot('oa:hasBody')(images[b64])
        )
      }
    }

    triples.forEach(triple => writer.addTriple(triple));
  });

  writer.end();
}
