import sys, shutil, os, stat

def debug_print(str):
	# print str
	pass

#-----------------------------------------------------
def shutil_chmod_folder(folderDir):
	write_perm = stat.S_IRUSR | stat.S_IWUSR | stat.S_IRGRP | stat.S_IROTH

	fileList = os.listdir(folderDir)

	for fileName in fileList:
		srcPath = folderDir + '/' + fileName
		
		if os.path.isdir(srcPath):
			shutil_chmod_folder(srcPath)
		else:
			perms = os.stat(srcPath)
			os.chmod(srcPath, perms.st_mode | write_perm)

#-----------------------------------------------------
def main(edition, debugB):

	if debugB:
		targetName = ' Debug'
	else:
		targetName = ''

	projName	= 'kJams ' + edition					# "kJams Pro"
	projTarget	= projName + targetName					# "kJams Pro Debug"
	appName		= projTarget + '.app'					# "kJams Pro Debug.app"
	targetApp	= 'build' + os.path.sep + appName		# "build/kJams Pro Debug.app"

	if not os.path.exists(targetApp):
		projName	= 'kJams X ' + edition					# "kJams X Pro"
		projTarget	= projName + targetName					# "kJams X Pro Debug"
		appName		= projTarget + '.app'					# "kJams X Pro Debug.app"
		targetApp	= 'build' + os.path.sep + appName		# "build/kJams X Pro Debug.app"

	if not os.path.exists(targetApp):
		return

	destRes		= targetApp + '/Contents/Resources'
	
	if not os.path.exists(destRes):
		return
	
	print 'chmoding Resources...'
	
	print destRes
	shutil_chmod_folder(destRes)
	
#-----------------------------------------------------
if __name__ == "__main__":
	args = sys.argv[1:]
	
	if len(args) != 1:
		print 'No config specified. Use -config $(ConfigurationName) in your commandline arg'
	else:
		target = args[0]
		print 'target: ' + target
		
		debugB = False

		if target.endswith('Debug'):
			debugB = True
			
		if target.startswith('Pro'):
			edition = 'Pro'
		elif target.startswith('Lite'):
			edition = 'Lite'
		elif target.startswith('2'):
			edition = '2'
		else:
			raise OSError("bad edition")
		
		main(edition, debugB)

