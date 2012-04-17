#!/usr/bin/env python

from os import environ

class Vector:
	def __init__(self, vec):
		(self.x, self.y, self.z) = vec
	def scale(self, scale):
		self.x *= scale
		self.y *= scale
		self.z *= scale
	def __repr__(self):
		return 'vec(%f, %f, %f)'%(self.x, self.y, self.z)

class Point:
	def __init__(self, (vert, norm)):
		assert(isinstance(vert, Vector))
		assert(isinstance(norm, Vector))
		self.vert = vert
		self.norm = norm
	def __repr__(self):
		return 'point(%s, %s)'%(repr(self.vert), repr(self.norm))

class Triangle:
	def __init__(self, points):
		assert(len(points) == 3)
		(self.a, self.b, self.c) = points
	def __repr__(self):
		return 'tri(%s, %s, %s)'%(repr(self.a),
					repr(self.b), repr(self.c))

class Obj:
	def v_handler(self, l, kw = None):
		vec = Vector(map(float, l.split(None, 2)))
		vec.scale(self.scale)
		self.verts.append(vec)
		return
	def vn_handler(self, l, kw = None):
		vec = Vector(map(float, l.split(None, 2)))
		self.norms.append(vec)
		return
	def f_handler(self, l, kw = None):
		f = map(lambda x:map(int, x.split('/', 2)), l.split())
		self.faces.append(tuple(f))
	def nul_handler(self, l, kw = None):
		return

	def point(self, point):
		(v, vt, n) = point
		return Point((self.verts[v - 1], self.norms[n - 1]))

	def tri(self, rec):
		return Triangle(map(self.point, rec))

	def quad2tri(self, q):
		(a, b, c, d) = q
		return [(a, b, d), (d, b, c)]

	def maketris(self):
		out = []
		for f in self.faces:
			if len(f) == 3:
				out.append(self.tri(f))
				continue
			if len(f) != 4:
				print f
				raise Exception('Cannot deal with polygons')
			tris = map(self.tri, self.quad2tri(f))
			out.extend(tris)
		self.tris = out

	def __init__(self, src, scale = 1.0):
		self.scale = scale
		self.verts = []
		self.norms = []
		self.faces = []
		h = {
			'v': self.v_handler,
			'vn': self.vn_handler,
			'f': self.f_handler,
		}
		while True:
			l = src.readline()
			if l == '':
				break
			l = l.rstrip('\r\n')
			if l == '' or l[0] == '#':
				continue

			l = l.split(None, 1)
			handler = h.get(l[0], self.nul_handler)
			handler(l[1], l[0])
		self.maketris()

def rip(src, dst):
	scale = float(environ.get('SCALE', '1.0'))
	o = Obj(src, scale)
	o.maketris()

	for t in o.tris:
		for p in (t.a, t.b, t.c):
			n = 'n %f %f %f\n'%(p.norm.x, p.norm.y, p.norm.z)
			v = 'v %f %f %f\n'%(p.vert.x, p.vert.y, p.vert.z)
			dst.write(n + v)

	print '%s -> %s'%(src.name, dst.name)
	print ' scale = %f, num_tris=%d'%(scale, len(o.tris))

def do_file(x):
	if len(x) < 4 or x[-4:].lower() != '.obj':
		raise Exception('%s: doesn\'t look like an obj file'%x)
	src = open(x)
	dst = open(x[:-4] + '.g', 'w')

	rip(src, dst)

def main(argv):
	for x in argv[1:]:
		try:
			do_file(x)
		except Exception, e:
			print '%s: %s'%(x, ': '.join(e.args))
			return False
	return True

if __name__ == '__main__':
	from sys import argv
	raise SystemExit, not main(argv)
