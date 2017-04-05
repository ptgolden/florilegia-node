{
	"targets": [
		{
			"target_name": "pdf2oac",
			"sources": [
				"cpp/pdf2oac.cc",
				"lib/ImageOutputDev.cc"
			],
			"libraries": [
				"<!@(pkg-config --libs poppler)",
			],
			"include_dirs": [
				"<!(node -e \"require('nan')\")",
				"<!(node -e \"require('streaming-worker-sdk')\")",
				"<!@(pkg-config poppler --cflags-only-I | sed s/-I//g)",
				"lib",

			],
			"cflags": [
				"-Wall",
				"-std=c++11",
				"<!@(pkg-config --cflags poppler)"
			]
		}
  ]
}
