{
	"targets": [
		{
			"target_name": "pdf2oac",
			"sources": [
				"cpp/pdf2oac.cc",
				"cpp/oac.cc"
			],
			"libraries": [
				"-lpoppler-glib",
				"-Wl,-rpath=<!(pwd)/libpoppler/lib/"
			],
			"include_dirs": [
				"<!(pwd)/libpoppler/include/poppler",
				"<!(node -e \"require('nan')\")",
				"<!(node -e \"require('streaming-worker-sdk')\")",
				"<!@(pkg-config poppler-glib --cflags-only-I | sed s/-I//g)",
				"lib"
			],
			"cflags": [
				"-Wall",
				"-std=c++11",
				"<!@(pkg-config --cflags-only-other poppler-glib)"
			]
		}
  ]
}
