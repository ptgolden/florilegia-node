"use strict";

const fs = require('fs')
    , path = require('path')
    , parseAnnots = require('./annots')
    , { annotTransform } = require('./oac')

module.exports = function (pdfFilename, opts) {
  pdfFilename = path.resolve(pdfFilename)

  try {
    fs.statSync(pdfFilename);
  } catch (e) {
    throw new Error(`No such file: '${pdfFilename}'`);
  }

  const annotStream = parseAnnots(pdfFilename);

  return annotStream.pipe(annotTransform(opts))
}
