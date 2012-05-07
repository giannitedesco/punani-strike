#!/usr/bin/env python

from os import environ, unlink
from os.path import dirname, join
from errno import ENOENT

class Vec3:
	def __init__(self, vec = None):
		if vec is None:
			self.x = 0.0
			self.y = 0.0
			self.z = 0.0
		else:
			(self.x, self.y, self.z) = vec
	def scale(self, scale):
		self.x *= scale
		self.y *= scale
		self.z *= scale
	def __repr__(self):
		return '%s(%f, %f, %f)'%(self.__class__.__name__,
					self.x, self.y, self.z)

class Vector(Vec3):
	pass
class Color(Vec3):
	pass

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

class Mtl:
	def __init__(self, name):
		self.name = name
		self.ambient = Color()
		self.diffuse = Color()
		self.specular = Color()
	def __repr__(self):
		if self.name is None:
			return 'Mtl:Default'
		else:
			return 'Mtl(%s)'%self.name

class MtlLib:
	def newmtl_handler(self, l, kw = None):
		name = l
		self.cur = Mtl(name)
		self.__d[name] = self.cur
	def ka_handler(self, l, kw = None):
		self.cur.ambient = Color(map(float, l.split()))
	def kd_handler(self, l, kw = None):
		self.cur.diffuse = Color(map(float, l.split()))
	def ks_handler(self, l, kw = None):
		self.cur.specular = Color(map(float, l.split()))
	def nul_handler(self, l, kw = None):
		return
	def __init__(self, f = None):
		self.__d = {}
		m = Mtl(None)
		m.diffuse = Color([1.0, 1.0, 1.0])
		self.__default = m
		if f is None:
			return
		h = {
			'newmtl': self.newmtl_handler,
			'Ka': self.ka_handler,
			'Kd': self.ks_handler,
			'Ks': self.kd_handler,
		}
		self.cur = None
		while True:
			l = f.readline()
			if l == '':
				break
			l = l.rstrip('\r\n')
			if l == '' or l[0] == '#':
				continue
			l = l.split(None, 1)
			handler = h.get(l[0], self.nul_handler)
			handler(l[1], l[0])
	def __getitem__(self, key):
		return self.__d.get(key, self.__default)

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
		f = map(lambda x:map(lambda x:len(x) and int(x) or None, \
				x.split('/', 2)), l.split())

		if sum(map(len, f)) != len(f) * 3:
			print l, f
			raise Exception('Missing normals')
		self.faces.append(tuple(f))
	def mtllib_handler(self, l, kw = None):
		d = dirname(self.fn)
		try:
			f = open(join(d, l))
		except IOError, e:
			if e.errno == ENOENT:
				self.lib = MtlLib()
				print 'WARNING: %s not found'%join(d,l)
				return
			else:
				raise
		self.lib = MtlLib(f)

	def usemtl_handler(self, l, kw = None):
		self.mtl = self.lib[l]

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
		self.fn = src.name
		self.scale = scale
		self.verts = []
		self.norms = []
		self.faces = []
		self.lib = MtlLib()
		self.mtl = self.lib[None]
		h = {
			'v': self.v_handler,
			'vn': self.vn_handler,
			'f': self.f_handler,
			'mtllib': self.mtllib_handler,
			'usemtl': self.usemtl_handler,
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
	try:
		if len(x) < 4 or x[-4:].lower() != '.obj':
			raise Exception('%s: doesn\'t look like an obj file'%x)
		src = open(x)
		dst = open(x[:-4] + '.g', 'w')
		rip(src, dst)
	except Exception, e:
		fn = dst.name
		del dst
		unlink(fn)
		raise
	return True

def main(argv):
	for x in argv[1:]:
		if not do_file(x):
			return False
	return True

if __name__ == '__main__':
	from sys import argv
	raise SystemExit, not main(argv)
