"use strict";

const fs = require('fs')
    , path = require('path')
    , parseAnnots = require('./annots')
    , { annotTransform } = require('./oac')

module.exports = function (pdfFilename, opts) {
  const { imageDirectory } = opts

  pdfFilename = path.resolve(pdfFilename)

  try {
    fs.statSync(pdfFilename);
  } catch (e) {
    throw new Error(`No such file: '${pdfFilename}'`);
  }

  return parseAnnots(pdfFilename, imageDirectory).pipe(annotTransform(opts))
}
