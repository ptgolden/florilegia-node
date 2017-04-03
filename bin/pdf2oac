#!/usr/bin/env node
// vim: set ft=javascript

const path = require('path')
    , tmp = require('tmp')
    , annotsToOAC = require('../oac')
    , { getAnnotations } = require('../')

const argv = require('yargs')
  .usage('Convert annotated PDF files to Web Annotation Data Model')
	.describe('pdf-uri', 'URI of PDF (defaults to filename)')
  .demandCommand(1)
  .help('h', 'help')
	.argv

argv._.forEach(filename => {
  const imageDirectory = tmp.dirSync()
      , annotations = getAnnotations(filename, imageDirectory.name)

  annotsToOAC(annotations, {
    imageDirectory: imageDirectory.name,
    pdfURI: 'file://' + path.resolve(filename)
  });
});

  /*




*/
