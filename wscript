#!/bin/python

from waflib import *
import os

top = '.'
out = 'build'

projname = 'boomfield'
coreprog_name = projname

g_cflags = ["-Wall", "-std=gnu11", "-pthread"]

def btype_cflags(ctx):
	return {
		"DEBUG"   : g_cflags + ["-Wextra", "-Og", "-ggdb3", "-march=core2", "-mtune=native"],
		"NATIVE"  : g_cflags + ["-Ofast", "-march=native", "-mtune=native"],
		"RELEASE" : g_cflags + ["-O3", "-march=core2", "-mtune=generic"],
	}.get(ctx.env.BUILD_TYPE, g_cflags)

def options(opt):
	opt.load("gcc")
	opt.add_option('--build_type', dest='build_type', type="string", default='RELEASE', action='store', help="DEBUG, NATIVE, RELEASE")

def configure(ctx):
	ctx.load("gcc")
	btup = ctx.options.build_type.upper()
	if btup in ["DEBUG", "NATIVE", "RELEASE"]:
		Logs.pprint("PINK", "Setting up environment for known build type: " + btup)
		ctx.env.BUILD_TYPE = btup
		ctx.env.CFLAGS = btype_cflags(ctx)
		Logs.pprint("PINK", "CFLAGS: " + ' '.join(ctx.env.CFLAGS))
	else:
		Logs.error("UNKNOWN BUILD TYPE: " + btup)

def build(bld):
	files = bld.path.ant_glob('src/*.c')
	bld.install_files('${PREFIX}/include/boomfield', ['src/tracer.h'])
	coreprog = bld (
		features = "c cshlib",
		target = coreprog_name,
		source = files,
		lib = [],
		uselib = []
	)
	
def clean(ctx):
	pass
