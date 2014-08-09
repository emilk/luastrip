import os
import sys
from subprocess import call

in_root  = "/Users/emilk/Documents/kod/AHGS/slayer/lua"
out_root = "output"

for file_root, subFolders, files in os.walk(in_root):
	for file in files:
		name, ext = os.path.splitext(file)
		if ext == ".lua":
			root_rel = os.path.relpath(file_root, in_root)

			out_dir = os.path.join(out_root, root_rel)
			if not os.path.exists(out_dir):
				os.makedirs(out_dir)

			in_path = os.path.join(file_root, file)
			out_path = os.path.join(out_dir, file)

			#print("file: " + file)
			#print("root_rel: " + root_rel)
			#print("in_path: " + in_path)
			#print("out_path: " + out_path)
			os.system("./solpp -o " + out_path + " " + in_path)
