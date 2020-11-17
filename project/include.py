import os, sys

def	get_script_path_local(fileStr):
	scriptPath = fileStr
	if scriptPath.find(os.sep) == -1:
		scriptPath = os.getcwd() + os.sep + scriptPath
	return scriptPath

def	get_script_folder_local():
	scriptFile = os.path.abspath(__file__)
	#debug_print("__file__: " + scriptFile)
	pathStr = get_script_path_local(scriptFile)
	#debug_print("script file: " + pathStr)
	pathStr = os.path.dirname(pathStr)
	#debug_print("script folder: " + pathStr)
	return os.path.abspath(pathStr) + '/'

sys.path.append(os.path.abspath(			\
	get_script_folder_local() + "../..")	\
	+ '/CF/python/')
