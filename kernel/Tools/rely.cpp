// find all needed *.c and *.S files excepts those in Tools/
// use gcc -MM for getting all the .h file that this file relies on.
// then output to .depend file
#include <string>
#include <cstring>
#include <iostream>
#include <dirent.h>
#include <format>
#include <vector>

std::vector<std::string> objList;
std::vector<std::string> tmpList;
std::string archName;

void searchFile(const std::string &path) {
	
	DIR *dir = opendir(path.c_str());
	dirent *ptr;
	if (dir == NULL) {
		printf("failed to open directory %s\n", path.c_str());
		return ;
	}
	while ((ptr = readdir(dir)) != NULL) {
		if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0)
			continue;
		else if (ptr->d_type == 0x8) {
			// printf("file     : %s/%s\n", path.c_str(), ptr->d_name);
			std::string filePath = path + "/" + ptr->d_name;
			std::string objPath = filePath;
			objPath[objPath.size() - 1] = 'o';
			std::string suf = filePath.substr(filePath.size() - 2);
			if (suf == ".c") {
				system(std::format("echo -n \"{0}/\" >> .depend", path).c_str());
				std::string cmd = std::format("gcc -MM {0} -I./includes/ | tee -a .depend > /dev/null", filePath);
				system(cmd.c_str());
				system(std::format("echo \"\\t@\\$(CC) \\$(C_FLAGS) -c {0} -o {1}\" >> .depend", filePath, objPath).c_str());
				system(std::format("echo \"\\t@echo \"[CC] {0}\"\" >> .depend", filePath).c_str());
				
				objList.push_back(objPath);
			} else if (suf == ".S") {
				std::string tmpPath = filePath;
				tmpPath.append("TMP");
				system(std::format("echo -n \"{0}/\" >> .depend", path).c_str());
				std::string cmd = std::format("gcc -MM {0} -I./includes/ | tee -a .depend > /dev/null", filePath);
				system(cmd.c_str());
				system(std::format("echo \"\\t@\\$(CC) -E \\$(C_INCLUDE_PATH) {0} > {1}\" >> .depend", filePath, tmpPath).c_str());
				system(std::format("echo \"\\t@\\$(ASM) \\$(ASM_FLAGS) -o {0} {1}\" >> .depend", objPath, tmpPath).c_str());
				system(std::format("echo \"\\t@echo \"[ASM] {0}\"\" >> .depend", filePath).c_str());

				objList.push_back(objPath);
				tmpList.push_back(tmpPath);
			}
			
		} else if (ptr->d_type == 0x10) {
			// printf("link file: %s/%s\n", path.c_str(), ptr->d_name);
		} else {
			std::string subPath = path;
			subPath.push_back('/');
			subPath.append(ptr->d_name);
			if (path == "./hal" && subPath != "./hal/" + archName) continue;
			searchFile(subPath);
		}
	}
}

int main(int argc, char **argv) {
	if (argc != 3 || (strcmp(argv[1], "-arch") != 0)) {
		printf("Error: format: -arch [arch_name]");
		return -1;
	}
	archName = std::string(argv[2]);
	printf("arch : %s\n", archName.c_str());
	system("rm includes/hal");
	system(std::format("ln -s ../hal/{0}/includes includes/hal", archName).c_str());
	system("echo \"\" > .depend");
	searchFile(".");
	std::string allObj = "ALLOBJS=";
	for (const auto &obj : objList) allObj.append(obj + " ");
	system(std::format("echo \"{0}\" >> .depend", allObj).c_str());
	std::string allTmp = "ALLTMPS=\\$(ALLOBJS) ";
	for (const auto &tmp : tmpList) allTmp.append(tmp + " ");
	system(std::format("echo \"{0}\" >> .depend", allTmp).c_str());
	return 0;
}