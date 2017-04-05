"use strict";

const test = require('blue-tape')
    , $ = require('rdf-builder')({ prefixes: require('../prefixes') })
    , oac = require('../oac')

const commentAnnot = {
  page: 4,
  motivation: 'commenting',
  body_text: 'This is a comment',
}

test('Converting annotation to triples', t => {
  const triples = oac.annotToTriples(commentAnnot, {
    baseURI: '',
    pdfURI: 'ex:pdf',
  });

  t.deepEqual(
    triples.slice(0, 2),
    $('annotation-0')({
      'rdf:type': 'oa:Annotation',
      'oa:hasMotivation': 'oa:commenting',
    }),
    'should generate an oa:Annotation from a comment representation'
  );

  return Promise.resolve();
})


