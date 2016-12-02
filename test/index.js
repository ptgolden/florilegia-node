"use strict";

const fs = require('fs')
    , path = require('path')
    , test = require('blue-tape')
    , { getAnnotations } = require('../')

function getPdfCreator(filename) {
  return filename.replace(/.*\//, '').replace('.pdf', '');
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
          body_text: 'This is a comment',
      }
    ],
    msg: 'should extract comment annotations'
  },

  {
    dir: './pdfs/2_highlight',
    expected: [
      {
          page: 1,
          motivation: 'highlighting',
          highlighted_text: 'document',
      }
    ],
    msg: 'should extract highlight annotations'
  },

  {
    dir: './pdfs/3_highlight_multiline',
    expected: [
      {
          page: 1,
          motivation: 'highlighting',
          highlighted_text: 'that spans',
      }
    ],
    msg: 'should extract multiline highlight annotations'
  }
]

test('Extracting annotations', t => {
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
