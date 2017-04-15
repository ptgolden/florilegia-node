#!/usr/bin/env node
// vim: set ft=javascript

const path = require('path')
    , N3 = require('n3')
    , pump = require('pump')
    , jsonld = require('jsonld')
    , concat = require('concat-stream')
    , JSONStream = require('JSONStream')
    , pdf2oac = require('./')
    , prefixes = require('./prefixes')

const argv = require('yargs')
  .usage('Convert annotated PDF files to Web Annotation Data Model')
  .command('<filename>', 'filename of PDF to convert')
	.describe('pdf-uri', 'URI of PDF (default: file://$filename)')
	.describe('graph-uri', 'URI of named graph (default: undefined)')
  .option('format', {
    alias: 'f',
    describe: 'Output format for RDF',
    choices: ['turtle', 'trig', 'n-triples', 'n-quads', 'json', 'jsonld'],
    default: 'turtle'
  })
  .option('base-uri', {
    alias: 'b',
    describe: 'Base URI for all generated triples (default: http://example.org#)'
  })
  .demandCommand(1)
  .help('h', 'help')
	.argv

argv._.map(p => path.resolve(p)).forEach(pdfFilename => {
  const tripleStream = pdf2oac(pdfFilename, {
    pdfURI: argv['pdf-uri'] || 'file://' + pdfFilename,
    baseURI: argv['base-uri'],
    graphURI: argv['graph-uri']
  })

  let rdfStream = pump(
      tripleStream,
      argv.format === 'json'
        ? JSONStream.stringify()
        : N3.StreamWriter({
            prefixes,
            format: argv.format === 'jsonld' ? 'ntriples' : argv.format,
            end: false
          }))

  if (argv.format === 'jsonld') {
    rdfStream = pump(rdfStream, concat((triples) => {
      jsonld.fromRDF(triples, { format: 'application/nquads' }, (err, doc) => {
        if (err) throw err;
        process.stdout.write(JSON.stringify(doc, true, '  ') + '\n')
      })
    }))
  } else {
    pump(rdfStream, process.stdout)
      .on('error', err => {
        if (err.code === 'EPIPE') {
          process.exit(0);
        }

        throw err;
      })
  }
});
