# About
Geh is a simple commandline image viewer written in C/Gtk+ with
various nice features. It currently supports the following modes:

* Slide-show mode displaying multiple images in a row either
  controlled by an time interval and/or mousebutton clicks.
* Thumbnail mode creating small thumbnail images and caching them
  in _~/.thumbnails_ according to freedesktop.org's thumbnail
  specification.

For more information check out the geh project page at:
https://github.com/pekdon/geh

```
git clone https://github.com/pekdon/geh.git
mkdir -p geh/build
cd geh/build
cmake ..
make
make install
```

## Usage

### Keybindings

* _f_, zoom to fit.
* _F_, full mode showing only the current picture.
* _s_, slide mode showing a large picture and one row of thumbnails.
* _S_, open save picture dialog.
* _R_, open rename picture dialog.
* _t T_, thumbnail mode showing only thumbnails.
* _q Q_, quit.
* _n N_, show/select next image.
* _p P_, show/select previous image.
* _+_, zoom in current image.
* _-_, zoom out current image.
* _0_, zoom current image to original size.
* _F11_, toggle fullscreen mode.

