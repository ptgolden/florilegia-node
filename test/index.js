"use strict";

const fs = require('fs')
    , path = require('path')
    , test = require('tape')
    , { getAnnotations } = require('../build/Release/pdf2oac')


test('Extracting annotations', t => {
  t.plan(2);

  const a = getAnnotations(path.join(__dirname, './none.pdf'));
  t.deepEqual(a, [], 'should find no annotations on a document without them.');

  const b = getAnnotations(path.join(__dirname, './comment.pdf'));
  t.deepEqual(b, [{
    page: 1,
    motivation: 'commenting',
    body_type: 'text',
    body_text: 'This is a comment',
    body_label: null
  }]);
})
