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

function annotsToOAC(user, filename, cb) {
  const writer = new Writer({ prefixes: PREFIXES, format: 'turtle' })
      , annotations = getAnnotations(filename)
      , triples = []

  const addTriple = (subject, predicate, object) => {
    triples.push({ subject, predicate, object })
  }

  annotations.forEach((annotation, i) => {
    const { page, motivation, body_text, highlighted_text } = annotation
        , annotID = makeTag(user, filename, `annot-${i + 1}`)
        , makeURI = p => `${annotID}/${p}`
        , targetID = makeURI('target')
        , pdfSelectorID = makeURI('page-selector')

    addTriple(
      annotID,
      'rdf:type',
      'oa:Annotation');

    addTriple(
      annotID,
      'oa:hasMotivation',
      'oa:' + motivation);

    addTriple(
      annotID,
      'oa:hasTarget',
      targetID);

    addTriple(
      targetID,
      'oa:hasSource',
      'file://' + filename);

    // PDF Selector
    addTriple(
      targetID,
      'oa:hasSelector',
      pdfSelectorID);

    addTriple(
      pdfSelectorID,
      'rdf:type',
      'oa:FragmentSelector');

    addTriple(
      pdfSelectorID,
      'rdf:value',
      Util.createLiteral(`#page=${page}`));

    addTriple(
      pdfSelectorID,
      'oa:conformsTo',
      'http://tools.ietf.org/rfc/rfc3778');

    // Text selector
    if (highlighted_text) {
      const textSelectorID = makeURI('text-selector')

      addTriple(
        targetID,
        'oa:hasSelector',
        textSelectorID);

      addTriple(
        textSelectorID,
        'rdf:type',
        'oa:TextQuoteSelector');

      addTriple(
        textSelectorID,
        'oa:exact',
        Util.createLiteral(highlighted_text));
    }

    // Text body (i.e. comment)
    if (body_text) {
      const commentBodyID = makeURI('comment-body')

      addTriple(
        annotID,
        'oa:hasBody',
        commentBodyID);

      addTriple(
        commentBodyID,
        'dc:format',
        Util.createLiteral('text/plain'));

      addTriple(
        commentBodyID,
        'rdf:type',
        'dctypes:Text');

      addTriple(
        commentBodyID,
        'rdf:type',
        'cnt:ContentAsText');

      addTriple(
        commentBodyID,
        'cnt:chars',
        Util.createLiteral(body_text))
    }

    triples.forEach(triple => writer.addTriple(triple));
  });

  writer.end(cb);
}

module.exports = { annotsToOAC }
