{
	"targets": [
		{
			"target_name": "pdf2oac",
			"sources": [
				"src/pdf2oac.cc",
				"lib/ImageOutputDev.cc"
			],
			"libraries": [
				"<!@(pkg-config --libs poppler)",
			],
			"include_dirs": [
				"<!(node -e \"require('nan')\")",
				"<!@(pkg-config poppler --cflags-only-I | sed s/-i//g)",
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
