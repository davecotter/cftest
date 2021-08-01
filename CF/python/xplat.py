import platform, subprocess, os

def debug_print(str):
	#print "xplat: " + str
	pass

def file_create_abs(filePath, contents = ""):
	with open(filePath, "w") as dstFile:
		if contents != "":
			dstFile.write(contents)

def	enable_separate_ppc_builds():
	# to have PPC be part of the build:
	# set to True, and run the 10.9 VM
	ppc_okB = False
	return ppc_okB or (not is_host_os() and is_mac())

def bid_depth_to_arch(bitDepth):
	arch = 'x'
	if bitDepth == '64':
		arch += bitDepth
	else:
		arch += '86'
		
	return arch

def system(cmdA):
	cmdStr = cmdA.pop(0)
	for subCmd in cmdA:
		if ' ' in subCmd:
			subCmd = '"' + subCmd + '"'
		cmdStr += ' ' + subCmd
	os.system(cmdStr)

def just_pipe(cmd):
	debug_print('pipe: ' + str(cmd));
	debug_print('pipe: about to pOpen');
	
	return subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

def pipe(cmd, print_outB = False):
	pipe = just_pipe(cmd)
	
	debug_print('pipe: about to communicate');
	
	(out, err) = pipe.communicate()

	debug_print('pipe: about to get returncode');

	ret = pipe.returncode
	
	errStr = str(err)

	if errStr != "":

		ignoreOutA = [
			"i386 architecture is deprecated"
		]
	
		showOutB = print_outB and out != ""
		
		if showOutB:
			for ignoreOut in ignoreOutA:
				if ignoreOut in out:
					showOutB = False

		if showOutB:
			print "out: <\n" + out + "\n> :out"

		ignoreErrsA = [
			"Cannot find Visual Studio",
			"kCFURLVolumeIsAutomountedKey missing",
			"DTDeviceKit: deviceType from",
			"iPhoneConnect",
			"usr/local/mysql",
			"usr/local/opt/libiodbc",
			"Postgres.app/Contents",
			"i386 architecture is deprecated"
		]

		showErrB = True
		
		for ignoreErr in ignoreErrsA:
			if ignoreErr in errStr:
				showErrB = False

		if showErrB:
			print "err: <" + errStr + "> :err"
		
	if ret != 0:
		print "return value: " + str(ret)
		raise OSError("error in build")

def include_separate_ppc_builds():
	return True and enable_separate_ppc_builds()

def is_mac():
	return platform.system() == 'Darwin'

def get_os_vers():
	versStr = ''
	
	if is_mac():
		versStr = platform.mac_ver()[0]
	else:
		versStr = platform.win32_ver()[0]
		
	return versStr;
	
def is_host_os(): # as opposed to a VM (guest OS) or Windows
	'''
	if is_mac():
		print "it's a mac"
	else:
		print "NOT a mac"
	
	print get_os_vers()
	'''
	
	host_vers = get_os_vers();
	
	is_guestB = host_vers == "10.9.5" or host_vers == "10.6.8"
	
	'''
	if is_guestB:
		print "is guest"
	else:
		print "NOT guest"
	'''

	is_hostB = not is_mac() or not is_guestB
	
	'''
	if is_hostB:
		print "is host"
	else:
		print "NOT host"
	'''

	return is_hostB

def building_ppc():
	return enable_separate_ppc_builds() and not is_host_os()

def get_build_folder(forceB = False):
	build_folder = 'build/'
	
	if forceB or building_ppc():
		build_folder += 'ppc/'
	
	return build_folder

