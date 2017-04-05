#!/usr/bin/env node
// vim: set ft=javascript

const path = require('path')
    , pdf2oac = require('./')
    , prefixes = require('./prefixes')
    , { Writer } = require('n3')

const argv = require('yargs')
  .usage('Convert annotated PDF files to Web Annotation Data Model')
  .command('<filename>', 'filename of PDF to convert')
	.describe('pdf-uri', 'URI of PDF (default: file://$filename)')
	.describe('graph-uri', 'URI of named graph (default: undefined)')
  .option('format', {
    alias: 'f',
    describe: 'Output format for RDF',
    choices: ['turtle', 'trig', 'n-triples', 'n-quads', 'json'],
    default: 'turtle'
  })
  .option('base-uri', {
    alias: 'b',
    describe: 'Base URI for all generated triples (default: http://example.org#)'
  })
  .demandCommand(1)
  .help('h', 'help')
	.argv

const writer = new Writer(process.stdout, {
  prefixes,
  end: false,
  format: argv.format
})

argv._.map(p => path.resolve(p)).forEach(pdfFilename => {
  const stream = pdf2oac({
    pdfFilename,
    pdfURI: argv['pdf-uri'] || 'file://' + pdfFilename,
    baseURI: argv['base-uri'],
    graphURI: argv['graph-uri']
  })

  stream.on('data', triple => {
    writer.addTriple(triple);
  }).on('end', () => {
    writer.end();
  })
});
