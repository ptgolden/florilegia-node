
POPPLER_VERSION := 0.54.0

POPPLER_SRC_DIR := poppler-$(POPPLER_VERSION)
POPPLER_ARCHIVE_FILE := $(POPPLER_SRC_DIR).tar.xz
POPPLER_ARCHIVE_URL := https://poppler.freedesktop.org/$(POPPLER_ARCHIVE_FILE)

PATCHES := $(wildcard patches/*.patch)

DIR := $(CURDIR)

all: lib/libpoppler-glib.so

clean:
	rm -rf include lib $(POPPLER_SRC_DIR) $(POPPLER_ARCHIVE_FILE) share src

$(POPPLER_ARCHIVE_FILE):
	wget --continue $(POPPLER_ARCHIVE_URL)

$(POPPLER_SRC_DIR): $(POPPLER_ARCHIVE_FILE) $(PATCHES)
	tar -xJf $<
	cd $(POPPLER_SRC_DIR) && for patch in $(PATCHES); do patch -p1 < $(DIR)/$$patch; done

lib/libpoppler-glib.so: $(POPPLER_SRC_DIR) $(PATCHES)
	cd $< && ./configure --prefix=$(DIR) --disable-poppler-qt4 --disable-poppler-cpp --disable-poppler-qt5 --disable-splash-output --disable-utils --disable-gtk-test
	cd $< && make -j --silent
	cd $< && make install --silent
