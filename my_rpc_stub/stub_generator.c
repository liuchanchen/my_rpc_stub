#include "stub_generator.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "unistd.h"
#include <fcntl.h>
#include <string.h>
#include <fcntl.h>

#define FILE_LEN (3 * 1024 * 1024)
#define FIND 0
#define NO_FIND -1
#define FAILED -1

typedef struct
{
	unsigned int current_line_num;
	unsigned int total_line_num;
}line_num_t;

typedef enum
{
	DSP_STUB=1,
	ARM_STUB,
	X_file
}file_type_e;

const char * file_path = "./dsp_stub.c";
char para_name[32][8] = { "a0", "a1","a2","a3", "a4", "a5" };
char *para_name_str = para_name;

static line_num_t dsp_stub_line, arm_stub_line, x_file_line;

static int find_specific_string_in_one_line(unsigned char *str, unsigned char * match_string, unsigned int string_len);
static int inline match_specific_character(unsigned char letter, unsigned char character);
static int inline foreach_and_find_specific_character_in_one_line(unsigned char*const str, const unsigned char character);
static int change_to_another_line(unsigned char * file_content, line_num_t line_num);
static int find_first_letter_after_specific_string_in_one_line(unsigned char *str, unsigned char * match_string, unsigned int string_len);
static int parse_var_in_one_line(int write_fd, unsigned char * read_context);
static int wrap_line(int write_fd, unsigned int file_type);

static int init_line_num(line_num_t * line_num);

static int add_line_num(line_num_t line_num);


int stub_generator(unsigned char*argv)
{
	int dsp_stub_fd = -1;
	int interface_file_fd = -1;
	unsigned char read_context[FILE_LEN];
	unsigned int parse_len = 0;
	int ret = -1;
	unsigned int str_len = 0;

	//file line number initialize
	init_line_num(&dsp_stub_line);
	init_line_num(&x_file_line);
	memset(&read_context, 0x0, sizeof(read_context));
	dsp_stub_fd = open(file_path, O_RDWR | O_CREAT, 0777);
	if (dsp_stub_fd < 0)
	{
		printf("no such file!\n");
		close(dsp_stub_fd);
		return -1;
	}
	printf("argv:%s\n", argv);
	interface_file_fd = open(argv, O_RDONLY, 0777);
	if (interface_file_fd < 0)
	{
		printf("no such file!\n");
		close(interface_file_fd);
		return -1;
	}
	ret = read(interface_file_fd, read_context, sizeof(read_context));
	parse_len = parse_var_in_one_line(dsp_stub_fd, read_context);
	if (parse_len < 0)
	{
		return -1;
	}
	wrap_line(dsp_stub_fd, DSP_STUB);

	close(dsp_stub_fd);
	close(interface_file_fd);

	return 0;
}

static int parse_var_in_one_line(int write_fd, unsigned char * read_content)
{
	unsigned int str_len = 0;
	unsigned int read_cnt = 0;
	int ret = -1;

	str_len = strlen("var");
	read_cnt = 0;
	ret = find_specific_string_in_one_line(read_content, "var", str_len);
	read_cnt = read_cnt + str_len + ret;
	if (FAILED == ret)
	{
		return -1;
	}
	printf("read_cnt:%d\n", read_cnt);
	str_len = strlen("=");
	ret = find_specific_string_in_one_line(read_content + read_cnt, "=", str_len);
	read_cnt = read_cnt + str_len + ret;
	if (FAILED == ret)
	{
		return -1;
	}
	printf("read_cnt:%d\n", read_cnt);
	str_len = strlen(";");
	ret = find_specific_string_in_one_line(read_content + read_cnt, ";", str_len);
	if (FAILED == ret)
	{
		printf("forget ; in line:%d\n", dsp_stub_line.current_line_num);
		return -1;
	}
	write(write_fd, read_content + read_cnt, ret);
	write(write_fd, para_name_str, strlen(para_name_str));
	para_name_str = para_name_str + sizeof(para_name[0]);
	write(write_fd, " ;", strlen(" ;"));
	read_cnt = read_cnt + str_len + ret;
	ret = change_to_another_line(read_content + read_cnt, dsp_stub_line);
	//no need adding str_len because ret has been added in change_to_another_line
	read_cnt = read_cnt + ret;
	printf("read_cnt:%d\n", read_cnt);

	return read_cnt;
}


static int find_specific_string_in_one_line(unsigned char *str, unsigned char * match_string, unsigned int string_len)
{
	int ret = -1;

	while(1)
	{
		ret = foreach_and_find_specific_character_in_one_line(str, *match_string);
		printf("ret:%d,%d,%d\n", ret, strncmp(str + ret, match_string, string_len - 1), string_len);
		if(FAILED == ret)
		{
			printf("no find in this line\n");
			return FAILED;
		}
		else if (0 != strncmp(str + ret, match_string, string_len - 1))
		{
			str = str + ret;
		}
		else
		{
			return ret;
		}
	}

	return FAILED;
}

static int inline match_specific_character(unsigned char letter, unsigned char character)
{
	return (letter == character) ? (FIND) : (NO_FIND);
}

static int inline foreach_and_find_specific_character_in_one_line(unsigned char*const str, const unsigned char character)
{
	int ret = -1;
	unsigned int cnt = 0;
	unsigned char * string = str;

	while(NO_FIND == match_specific_character(*string, '\n'))
	{
		if (FIND == match_specific_character(*string, character))
		{
			return cnt;
		}
		string++;
		cnt++;
	}

	return ret;
}

static int inline change_to_another_line(unsigned char* file_content, line_num_t line_num)
{
	int ret = -1;
	unsigned int cnt = 0;
	unsigned char * string = file_content;

	while (NO_FIND == match_specific_character(*file_content, '\n'))
	{
		string++;
		cnt++;
	}
	//the index of first character in new line
	cnt++;
    //line numeber increase(both total and currnet)
	add_line_num(line_num);

	return cnt;
}

static int inline get_line_size(unsigned char*const str)
{
	int ret = -1;
	unsigned int cnt = 0;
	unsigned char * string = str;

	while (NO_FIND == match_specific_character(*string, '\n'))
	{
		string++;
		cnt++;
	}
	//add 'LF'
	cnt++;

	return cnt;
}

static int find_first_letter_after_specific_string_in_one_line(unsigned char *str, unsigned char * match_string, unsigned int string_len)
{
	int ret = -1;

	while (1)
	{
		ret = foreach_and_find_specific_character_in_one_line(str, *match_string);
		printf("ret:%d,%d,%d\n", ret, strncmp(str + ret, match_string, string_len - 1), string_len);
		if (FAILED == ret)
		{
			printf("no find in this line\n");
			return FAILED;
		}
		else if (0 != strncmp(str + ret, match_string, string_len - 1))
		{
			str = str + ret + 1;
		}
		else
		{
			return (ret + 1);
		}
	}

	return FAILED;
}

static int wrap_line(int write_fd, unsigned int file_type)
{
	write(write_fd, "\n", 1);
	if (DSP_STUB == file_type)
	{
		dsp_stub_line.total_line_num++;
	}

	return 0;
}

static int inline init_line_num(line_num_t * line_num)
{
	line_num->current_line_num = 1;
	line_num->total_line_num = 1;

	return 0;
}

static int inline add_line_num(line_num_t line_num)
{
	line_num.current_line_num++;
	line_num.total_line_num++;

	return 0;
}
