"use strict";

const fs = require('fs')
    , path = require('path')
    , test = require('blue-tape')
    , { getAnnotations } = require('../build/Release/pdf2oac')


function getPdfCreator(filename) {
  return filename.replace(/.*\//, '').replace('.pdf', '');
}

function testFiles(t, files, expected, msg) {
}

const cases = [
  {
    dir: './pdfs/0_none',
    expected: [],
    msg: 'should not find any annotations on an empty document'
  },

  {
    dir: './pdfs/1_comment',
    expected: [
      {
          page: 1,
          motivation: 'commenting',
          body_type: 'text',
          body_text: 'This is a comment',
          body_label: null
      }
    ],
    msg: 'should extract comment annotations'
  }
]

test('Extracting annotations', t => {
  const tests = []

  cases.forEach(({ dir, expected, msg }) => {
    const pdfs = fs.readdirSync(path.join(__dirname, dir))

    pdfs.forEach(filename => {
      t.deepEqual(
        getAnnotations(path.join(__dirname, dir, filename)),
        expected,
        `${msg} (${getPdfCreator(filename)}).`)
    })
  })

  return Promise.resolve();
})
