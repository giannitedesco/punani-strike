#!/usr/bin/env python

from PIL import Image, ImageFont, ImageDraw
from os.path import basename

def rip_font(ffn, bfn):
	font = ImageFont.truetype(ffn, 48)
	print 'ripping %s -> %s'%(ffn, bfn)

	xsz = ysz = 0
	for x in xrange(16):
		for y in xrange(16):
			val = chr(y * 16 + x)
			(xx, yy) = font.getsize(val)
			if xx > xsz:
				xsz = xx
			if yy > ysz:
				ysz = yy

	im = Image.new('RGB', (xsz * 16, ysz * 16))
	draw = ImageDraw.Draw(im)

	for x in xrange(16):
		for y in xrange(16):
			val = chr(y * 16 + x)
			draw.text((x * xsz, y * ysz), val, font=font)

	im.save(bfn, optimize = True)

def main(argv):
	for x in argv[1:]:
		if len(x) < 4 or x[-4:].lower() != '.ttf':
			raise Exception('%s: doesn\'t look like an ttf file'%x)
		rip_font(x, './data/font/%s.png'%basename(x)[:-4])
	return True

if __name__ == '__main__':
	from sys import argv
	raise SystemExit, not main(argv)
