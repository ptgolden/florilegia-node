"use strict";

const path = require('path')
    , { Writer, Util } = require('n3')
    , { getAnnotations } = require('./')

const PREFIXES = {
  oa: "http://www.w3.org/ns/oa#",
  dc: "http://purl.org/dc/elements/1.1/",
  dctypes: "http://purl.org/dc/dcmitype/",
  cnt: "http://www.w3.org/2011/content#",
  bibo: "http://purl.org/ontology/bibo/",
  rdf: "http://www.w3.org/1999/02/22-rdf-syntax-ns#",
  rdfs: "http://www.w3.org/2000/01/rdf-schema#",
}

function makeTag(email, filename) {
  const d = new Date()

  return [
    'tag',
    email,
    `${d.getFullYear()}-${d.getMonth()}-${d.getDate()}`,
    path.basename(filename)
  ].join(':')
}

function format(val) {
  val = val.toString();

  if (Util.isLiteral(val)) {
    return val;
  }

  if (Util.isPrefixedName) {
    return Util.expandPrefixedName(val, PREFIXES)
  }
}

function tripleFactory(subject) {
  const func = predicate => object => ({
    subject: format(subject),
    predicate: format(predicate),
    object: format(object),
  })

  func.toString = () => subject;

  return func;
}

function pdf2oac(user='http://example.com', filename, cb) {
  const writer = new Writer({ prefixes: PREFIXES, format: 'ntriples' })
      , annotations = getAnnotations(filename)

  let triples = []

  annotations.forEach((annotation, i) => {
    const { page, motivation, body_text, highlighted_text } = annotation
        , annotURI = tripleFactory(makeTag(user, filename, `annot-${i + 1}`))
        , makeURI = p => `${annotURI}/${p}`
        , targetURI = tripleFactory(makeURI('target'))
        , pdfSelectorURI = tripleFactory(makeURI('page-selector'))

    triples = triples.concat([
      annotURI('rdf:type')('oa:Annotation'),
      annotURI('oa:hasMotivation')('oa:' + motivation),
      annotURI('oa:hasTarget')(targetURI),
      annotURI('oa:hasSource')('file://' + filename),
    ])

    triples = triples.concat([
      targetURI('oa:hasSelector')(pdfSelectorURI),
      pdfSelectorURI('rdf:type')('oa:FragmentSelector'),
      pdfSelectorURI('rdf:value')(Util.createLiteral(`#page=${page}`)),
      pdfSelectorURI('oa:conformsTo')('http://tools.ietf.org/rfc/rfc3778'),
    ])
    // Text selector
    if (highlighted_text) {
      const textSelectorURI = tripleFactory(makeURI('text-selector'))

      triples = triples.concat([
        targetURI('oa:hasSelector')(textSelectorURI),
        textSelectorURI('rdf:type')('oa:TextQuoteSelector'),
        textSelectorURI('oa:exact')(Util.createLiteral(highlighted_text)),
      ])
    }

    // Text body (i.e. comment)
    if (body_text) {
      const commentBodyURI = tripleFactory(makeURI('comment-body'))

      triples = triples.concat([
        annotURI('oa:hasBody')(commentBodyURI),
        commentBodyURI('dc:format')(Util.createLiteral('text/plain')),
        commentBodyURI('rdf:type')('dctypes:Text'),
        commentBodyURI('rdf:type')('cnt:ContentAsText'),
        commentBodyURI('cnt:chars')(Util.createLiteral(body_text)),
      ])
    }

    triples.forEach(triple => writer.addTriple(triple));
  });

  writer.end(cb);
}

module.exports = { pdf2oac }
