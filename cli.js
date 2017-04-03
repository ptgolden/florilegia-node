#!/usr/bin/env node
// vim: set ft=javascript

const path = require('path')
    , tmp = require('tmp')
    , streamify = require('stream-array')
    , annotsToOAC = require('./oac')
    , prefixes = require('./prefixes')
    , { Writer } = require('n3')
    , { getAnnotations } = require('./')


const argv = require('yargs')
  .usage('Convert annotated PDF files to Web Annotation Data Model')
  .command('<filename>', 'filename of PDF to convert')
	.describe('pdf-uri', 'URI of PDF (default: file://$filename)')
	.describe('graph-uri', 'URI of named graph (default: undefined)')
  .option('format', {
    alias: 'f',
    describe: 'Output format for RDF',
    choices: ['turtle', 'trig', 'n-triples', 'n-quads'],
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


argv._.forEach(filename => {
  const imageDirectory = tmp.dirSync()
      , annotations = getAnnotations(filename, imageDirectory.name)

  const oacStream = annotsToOAC({
    imageDirectory: imageDirectory.name,
    pdfURI: argv['pdf-uri'] || 'file://' + path.resolve(filename),
    baseURI: argv['base-uri'],
    graph: argv['graph-uri']
  }).on('data', triple => {
    writer.addTriple(triple);
  }).on('end', () => {
    writer.end();
  })

  streamify(annotations).pipe(oacStream)
});
