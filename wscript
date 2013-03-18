#!/usr/bin/env python

import subprocess
import time
import sys

top = "."
out = "build"

# change this stuff

APPNAME = "rutabaga"
VERSION = "0.1"

def separator():
	# just for output prettifying
	# print() (as a function) doesn't work in python <2.7
	sys.stdout.write("\n")

#
# dep checking functions
#

def check_plan9_extensions(conf):
	code = """
		struct parent {
			int type;
		};

		struct child {
			struct parent;
		};

		int return_type(struct parent *p)
		{
			return p->type;
		}

		int main(int argc, char **argv)
		{
			struct child c = {.type = 42};
			return (return_type(&c) == 42 ? 0 : 1);
		}"""

	try_flags = ["", "-fplan9-extensions"]

	for flags in try_flags:
		msg = ["Checking for Plan9 C Extensions"]

		if flags:
			msg.append("        (with {0})".format(flags))
		else:
			msg.append("        (without extra CFLAGS)")

		# all this msg[] shit is just a hack to make the lines
		# justify correctly when a configuration message has
		# a newline in it
		msg[1] += " " * (40 - len(msg[1]) )

		ret = conf.check_cc(
			mandatory=False,
			execute=True,
			fragment=code,
			cflags=flags,

			msg="\n".join(msg))

		# reset waf's line justification
		conf.line_just = 40

		if ret:
			conf.env.append_unique("CFLAGS", flags)
			return

	separator()
	conf.fatal("Your compiler doesn't seem to support the Plan9 C extensions.")

def check_alloca(conf):
	code = """
		#include <stdlib.h> /* *BSD have alloca in here */

		#ifdef NEED_ALLOCA_H
		#include <alloca.h>
		#endif

		int main(int argc, char **argv)
		{
			char *test = alloca(1);
			test[0] = 42;
			return 0;
		}"""

	if conf.check_cc(
		mandatory=False,
		execute=True,
		fragment=code,
		msg="Checking for alloca() in stdlib.h"):
		return

	if conf.check_cc(
		mandatory=True,
		execute=True,
		fragment=code,
		msg="Checking for alloca() in alloca.h",
		cflags=["-DNEED_ALLOCA_H=1"]):
		conf.define("NEED_ALLOCA_H", 1)

def pkg_check(conf, pkg):
	conf.check_cfg(
		package=pkg, args="--cflags --libs", uselib_store=pkg.upper())

def check_gl(conf):
	pkg_check(conf, "gl")

def check_freetype(conf):
	pkg_check(conf, "freetype2")

def check_x11(conf):
	check = lambda pkg: pkg_check(conf, pkg)

	check("x11-xcb")
	check("xcb")
	check("xcb-keysyms")
	check("xcb-icccm")

def check_jack(conf):
	conf.check_cc(
		mandatory=False,
		execute=True,

		lib="jack",
		header_name="jack/jack.h",
		uselib_store="JACK",
		errmsg="not found (won't build cabbage_patch)",

		msg="Checking for JACK")

#
# waf stuff
#

def options(opt):
	opt.load('compiler_c')

	rtb_opts = opt.add_option_group("rutabaga options")
	rtb_opts.add_option("--debug-layout", action="store_true", default=False,
			help="when enabled, objects will draw their bounds in red.")

def configure(conf):
	separator()
	conf.load("compiler_c")
	conf.load("gnu_dirs")

	# conf checks

	separator()
	check_plan9_extensions(conf)
	separator()
	check_alloca(conf)
	separator()

	check_gl(conf)
	check_freetype(conf)

	check_x11(conf)

	separator()

	check_jack(conf)

	separator()

	# setting defines, etc

	conf.env.VERSION = VERSION
	conf.define("_GNU_SOURCE", "")
	conf.env.append_unique("CFLAGS", [
		"-std=c99", "-ggdb", "-Wall", "-Werror",
		"-ffunction-sections", "-fdata-sections"])

	if conf.options.debug_layout:
		conf.env.RTB_LAYOUT_DEBUG = True
		conf.define("RTB_LAYOUT_DEBUG", True)

def build(bld):
	bld.recurse("src")
	bld.recurse("examples")
