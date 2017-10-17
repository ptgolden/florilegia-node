{
	"targets": [
		{
			"target_name": "pdf2oac",
			"sources": [
				"cpp/pdf2oac.cc",
				"cpp/oac.cc"
			],
			"libraries": [
				"-lpoppler-glib -Llibpoppler/lib"
			],
			"conditions": [
				['OS=="linux"', { 'libraries': [
					"-Wl,-rpath='$$ORIGIN'/../../libpoppler/lib/"
				]}],
				['OS=="mac"', { 'libraries': [
					"-Wl,-rpath=@rpath/../../libpoppler/lib/"
				]}]
			],
			"include_dirs": [
				"<!(pwd)/libpoppler/include/poppler",
				"<!(node -e \"require('nan')\")",
				"<!(node -e \"require('streaming-worker-sdk')\")",
			],
			"cflags": [
				"-Wall",
				"-std=c++11",
				"<!@(pkg-config glib-2.0 cairo --cflags)",
			]
		}
  ]
}
