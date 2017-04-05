"use strict";

const parseAnnots = require('./annots')
    , annotsToRDF = require('./oac')

module.exports = function (opts) {
  return parseAnnots(opts).pipe(annotsToRDF(opts))
}
