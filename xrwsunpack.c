/*
 XRWSunpack - Unpack X Rebirth Workshop .dat files downloaded from Steam
 Developed by Lit (https://github.com/Lighting/XRWSunpack)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define MAXSIZE 520000
#define XRWS_SIGNATURE "XRWS"
#define XRWS_VERSION 1

const char *xrwsunpack_header = "XRWSunpack v0.1 (" __DATE__ " " __TIME__ ")";

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
	fprintf(stderr, "Usage: %s [option] .dat_file [OUT_DIR]\n", argv[0]);
	fprintf(stderr, "Unpack X Rebirth Workshop (XRWS) .dat files downloaded from Steam\n");
	fprintf(stderr, "%s -h or --help for help\n\n");
	fprintf(stderr, "Report bugs to <https://github.com/Lighting/XRWSunpack/issues>\n");
}

void unpack(const char *file, const char *out_dir)
{
	FILE *ifd;
	struct header_struct {
		char sig[4];
		unsigned long ver;
		unsigned long files_number;
		unsigned long files_names_len;
		unsigned long files_size;
//		void swap()
//		{
//			ver = ntohl(ver);
//			files_number = ntohl(files_number);
//			files_names_len = ntohl(files_names_len);
//			files_size = ntohl(files_size);
//		}
	} header;
	unsigned long *files_sizes;
	char *files_names;
	
	//open XRWS file for reading
	ifd = fopen(file, "rb");
	if(ifd == NULL)
		terminate("Cannot open XRWS file %s", file);
	
	//read and check header
	fread(&header, 1, sizeof(header), ifd);
//	header.swap();
	if(strncmp(header.sig, XRWS_SIGNATURE, sizeof(header.sig)) != 0)
		terminate("%s is not a XRWS file", file);
	if(header.ver != XRWS_VERSION)
		terminate("%s have unsupported XRWS version %d, %d, %d, %d", file, header.ver, header.files_number, header.files_names_len, header.files_size);
	
	//read sizes of files
	files_sizes = malloc(header.files_number * 4);
	memset(files_sizes, 0, sizeof(files_sizes));
	fread(files_sizes, 1, sizeof(files_sizes), ifd);
  
	//read names of files
	files_names = malloc(header.files_names_len);
	memset(files_names, 0, sizeof(files_names));
	fread(files_names, 1, sizeof(files_names), ifd);
	
	fclose(ifd);
	
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
	
	if(strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
		usage(argv);
	else
		unpack(argv[1], (argc > 2) ? argv[2] : "-");

	return 0;
}
