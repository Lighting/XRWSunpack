/*
 XRWSunpack - Unpack X Rebirth Workshop .dat files downloaded from Steam
 Developed by Lit (https://github.com/Lighting/XRWSunpack)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

#define MAXSIZE 520000
#define XRWS_SIGNATURE "XRWS"
#define XRWS_VERSION 1
#define CONTENT_XML "content.xml"
#define PARSE_DAT ".dat"
#define PARSE_EXTENSIONS "extensions_"
#define PARSE_EXTENSIONS_LEN 11
#define PARSE_VERSION 'v'
#define PARSE_VERSION_TOKEN '_'
#define PARSE_WARNING "\nDont change file name after downloading from Steam"

#ifdef __linux__
	#define MAKEDIR(a) mkdir(a, 0775)
#else
	#define MAKEDIR(a) _mkdir(a)
#endif

#define SWAP_UINT32(x) (((x) >> 24) | (((x) & 0x00FF0000) >> 8) | (((x) & 0x0000FF00) << 8) | ((x) << 24))

const char *xrwsunpack_header = "XRWSunpack v1.0 (" __DATE__ " " __TIME__ ")";

void terminate(const char *fmt, ...)
{
	va_list ap;

	fprintf(stderr, "Error: ");
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, "\n");
	exit(1);
}

void usage(char **argv)
{
	fprintf(stderr, "%s\n", xrwsunpack_header);
	fprintf(stderr, "Usage: %s extension.dat [OUT_DIR]\n", argv[0]);
	fprintf(stderr, "Unpack X Rebirth Workshop (XRWS) .dat files downloaded from Steam\n\n");
	fprintf(stderr, "To download extention.dat files from Steam Workshop use http://steamworkshopdownloader.com\n");
	fprintf(stderr, "Report bugs to <https://github.com/Lighting/XRWSunpack/issues>\n");
}

void unpack(const char *file, const char *out_dir)
{
	FILE *ifd, *ofd;
	struct header_struct {
		char sig[4];
		unsigned int ver;
		unsigned int files_number;
		unsigned int files_names_len;
		unsigned int files_size;
	} header;
	unsigned int *files_sizes, files_names_pos;
	char *files_names, *data, *pointer, *pointer2;
	unsigned long read_size, len;
	char out_name[FILENAME_MAX*2], out_path[FILENAME_MAX*2];
	
	//check output directory
	if(*out_dir != '\0' && access(out_dir, W_OK) == -1)
		terminate("No access to directory %s", out_dir);

	//remove prefix path from name of file
	pointer = strrchr(file, '/');
	(pointer == NULL) ? pointer = file : pointer++;
	pointer2 = strrchr(pointer, '\\');
	(pointer2 == NULL) ? pointer2 = pointer : pointer2++;
	//remove prefix text from name of file
	pointer = strstr(pointer2, PARSE_EXTENSIONS);
	if(pointer != pointer2)
		terminate("Name of file %s must begin with \"%s\"%s", file, PARSE_EXTENSIONS, PARSE_WARNING);
	pointer += PARSE_EXTENSIONS_LEN;
	//remove extention and check it
	pointer2 = strrchr(pointer, '.');
	if(pointer2 == NULL || strcmp(pointer2, PARSE_DAT) != 0)
		terminate("Extension of file %s must be \"%s\"%s", file, PARSE_DAT, PARSE_WARNING);
	strncpy(out_name, pointer, pointer2 - pointer);
	out_name[pointer2 - pointer] = '\0';
	//remove version from name of file
	pointer = strrchr(out_name, PARSE_VERSION_TOKEN);
	if(pointer == NULL || pointer[1] != PARSE_VERSION)
		terminate("Name of file %s must contain version number%s", file, PARSE_WARNING);
	pointer[0] = '\0';

	//open XRWS file for reading
	ifd = fopen(file, "rb");
	if(ifd == NULL)
		terminate("Cannot open XRWS file %s", file);
	
	//read header
	fread(&header, 1, sizeof(header), ifd);
	//convert little-big endian uint
	header.ver = SWAP_UINT32(header.ver);
	header.files_number = SWAP_UINT32(header.files_number);
	header.files_names_len = SWAP_UINT32(header.files_names_len);
	header.files_size = SWAP_UINT32(header.files_size);
	if(strncmp(header.sig, XRWS_SIGNATURE, sizeof(header.sig)) != 0)
		terminate("%s is not a XRWS file", file);
	if(header.ver != XRWS_VERSION)
		terminate("%s have unsupported XRWS version %u", file, header.ver);
	
	//read sizes of files
	files_sizes = malloc(header.files_number * 4);
	memset(files_sizes, 0, sizeof(files_sizes));
	fread(files_sizes, header.files_number, 4, ifd);
	//convert little-big endian uint
	for(unsigned long counter = 0; counter < header.files_number; counter++)
		files_sizes[counter] = SWAP_UINT32(files_sizes[counter]);
	
	//read names of files
	files_names = malloc(header.files_names_len);
	memset(files_names, 0, header.files_names_len);
	fread(files_names, 1, header.files_names_len, ifd);
	
	//create extention subdirectory
	if(*out_dir != '\0')
		snprintf(out_path, sizeof(out_path), "%s/%s", out_dir, out_name);
	else
		strcpy(out_path, out_name);
	if(MAKEDIR(out_path) != 0)
		terminate("Connot create directory %s", out_path);
	
	//create files
	data = malloc(MAXSIZE);
	files_names_pos = 0;
	for(unsigned long counter = 0; counter < header.files_number; counter++)
	{
		snprintf(out_name, sizeof(out_name), "%s/%s", out_path, files_names + files_names_pos);
		ofd = fopen(out_name, "wb");
		if(ofd == NULL)
			terminate("Cannot create file %s", out_name);

		while((read_size = (files_sizes[counter] - ftell(ofd) > MAXSIZE) ? MAXSIZE : (files_sizes[counter] - ftell(ofd))) > 0)
		{
			len = fread(data, 1, read_size, ifd);
			fwrite(data, 1, len, ofd);
		}
		
		fclose(ofd);
		files_names_pos += strlen(files_names + files_names_pos) + 1;
	}
	
	//check data size with header
	if(ftell(ifd) != (sizeof(header) + header.files_names_len + (header.files_number * 4) + header.files_size))
		terminate("File %s corrupted", file);
	
	//create content.xml
	snprintf(out_name, sizeof(out_name), "%s/%s", out_path, CONTENT_XML);
	ofd = fopen(out_name, "wb");
	if(ofd == NULL)
		terminate("Cannot create file %s", out_name);
	while((len = fread(data, 1, MAXSIZE, ifd)) > 0)
		fwrite(data, 1, len, ofd);
	fclose(ofd);

	fclose(ifd);
	free(data);
	free(files_sizes);
	free(files_names);
}

int main(int argc, char **argv)
{
	if(argc < 2)
	{
		usage(argv);
		terminate("\nXRWS file not found");
	}
	
	if(strcmp(argv[1], "--help") == 0)
		usage(argv);
	else
		unpack(argv[1], (argc > 2) ? argv[2] : "");

	return 0;
}
