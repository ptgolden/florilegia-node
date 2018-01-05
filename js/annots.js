"use strict";

const { Readable } = require('stream')
    , { extract }  = require('bindings')('pdf2oac.node')

module.exports = function parseAnnots(pdfFilename) {
  let startedExtracting = false
    , finishedExtracting = false
    , finished = false
    , extractedAnnots

  return new Readable({
    objectMode: true,
    read() {
      if (!startedExtracting) {
        startedExtracting = true;
        extract(pdfFilename, (err, annots) => {
          if (err) {
            this.emit('error', err);
            finished = true;
            return
          }

          finishedExtracting = true;
          extractedAnnots = annots;

          if (!extractedAnnots.length) {
            this.push(null);
          } else {
            this.push(extractedAnnots.shift());
          }
        })
      } else if (finishedExtracting && !finished) {
        while (extractedAnnots.length) {
          if (!this.push(extractedAnnots.shift())) {
            break;
          }
        }

        if (!extractedAnnots.length) {
          this.push(null);
        }
      }
    }
  })
}
