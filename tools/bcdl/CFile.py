import pycparser
class CFile(object):
	def __init__(self, filename):
		print "#include<%s>"%filename
	def __getattr__(self, func):
		def _render(*args):
			arglist = []
			for arg in args:
				arglist.append(str(arg))
			return "%s(%s)"%(func, ", ".join(arglist))
		return _render
if __name__ == "__main__":
	frame = CFile("/cesk/cesk_frame.h")
	print frame.cesk_frame_new('x', 'y', 'z')


