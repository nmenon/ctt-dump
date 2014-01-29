/*
 * CTT DUMP Utility
 *
 * Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/
 *	Nishanth Menon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MEM_SIZE 4096
#define MEM_MASK (MEM_SIZE - 1)
#define MEMORY "/dev/mem"

#define ERRS(fmt, ...) \
	fprintf(stderr, "%s:%s:FAIL(%d): " fmt, app_name, __func__, errno, ##__VA_ARGS__);\
	perror("Operation Fail description");

#define ERRF(fmt, ...) fprintf(stderr, "%s:%s: " fmt, app_name, __func__, ##__VA_ARGS__)
#define ERR(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)


/**
 * struct reg_data - register info
 * @addr:	Address
 * @value:	Value
 */
struct reg_data {
	unsigned int addr;
	unsigned int value;
};

/**
 * struct rd1 - structure to store rd1 data in entirety
 * @device_name:	name of the device
 * @rdata:	empty data to show termination - list of register,address tuples
 */
struct rd1 {
	char device_name[200];
	struct reg_data *rdata;
};

static char *app_name = NULL;

#ifdef CONFIG_SANDBOX
/* Dummy register read.. for debug purposes */
static int readreg(unsigned int reg_addr, unsigned int *reg_val)
{
	*reg_val = 0xDEADBEEF;
	return 0;
}
#else
/**
 * readreg() - register read
 * @reg_addr:	register address
 * @reg_val:	returns register value
 *
 * returns 0 if all good, else returns non 0 value
 */
static int readreg(unsigned int reg_addr, unsigned int *reg_val)
{
	int fd, ret = 0;
	void *mem_map, *mem_addr;

	/* open device */
	fd = open(MEMORY, O_RDWR | O_SYNC);
	if (fd < 0) {
		ERRS("could not open \"%s\"!\n", MEMORY);
		return 1;
	}

	/* map memory */
	mem_map = mmap(0, MEM_SIZE,
		       PROT_READ | PROT_WRITE, MAP_SHARED,
		       fd, reg_addr & ~MEM_MASK);
	if (mem_map == (void *)-1) {
		ERRS("could not map memory!\n");
		return 1;
	}

	/* translate address */
	mem_addr = mem_map + (reg_addr & MEM_MASK);

	/* Read register */
	*reg_val = *((unsigned int *)mem_addr);

	/* unmap page and close file descriptor */
	ret = munmap(mem_map, MEM_SIZE);
	if (ret != 0)
		ERRS("mem unmap failed! (%d)\n", ret);
	close(fd);

	return 0;
}
#endif

/**
 * usage() - how to use this app
 * @err:	error string
 *
 * never returns...
 */
static void usage(char *err)
{
	if (err)
		ERR("ERROR: %s\n", err);
	ERR("Usage:\n");
	ERR("%s: -i input_file -o output_file\n", app_name);
	ERR("\t-i input_file: CTT generated dump-out file (rd1 format)\n");
	ERR("\t-o output_file: output file for read-in for CTT (rd1 format) - skip to dump to stdout\n");
	exit(EXIT_FAILURE);
}

/**
 * read_input_file() - read input file
 * @in_file:	input file name
 */
static struct rd1 *read_input_file(char *in_file)
{
	FILE *in;
	struct rd1 *r = calloc(1, sizeof(struct rd1));
	int idx = 0;
	struct reg_data rdata;
	struct reg_data *t;
	char str1[100], str2[100];

	if (!r) {
		ERRF("no memory to allocate rd1 struct\n");
		goto err_exit;
	}

	in = fopen(in_file, "r");
	if (!in) {
		ERRS("Unable to open \"%s\"\n", in_file);
		goto err_exit;
	}

	if (feof(in)) {
		ERRF("invalid file format(empty file?) in \"%s\"!\n", in_file);
		goto err_exit;
	}

	if (fscanf(in, "%s %s", str1, str2) != 2) {
		ERRS("invalid file format(DeviceName Actual device name) in \"%s\"!\n", in_file);
		goto err_exit;
	}
	snprintf(r->device_name, 200, "%s %s", str1, str2);

	while (!feof(in)) {
		int ret = 0;
		ret = fscanf(in, "%s %s", str1, str2);
		if (ret == EOF)
			break;
		if (ret != 2) {
			ERRS("invalid file format (addr data) in \"%s\" - expected 2, got %d!\n", in_file, ret);
			goto err_exit;
		}
		ret = sscanf(str1, "%x", &rdata.addr);
		if (ret != 1) {
			ERRF("invalid addr '%s' at Line %d in '%s' file\n",
			     str1, idx + 2, in_file);
			goto err_exit;
		}
		ret = sscanf(str2, "%x", &rdata.value);
		if (ret != 1) {
			ERRF("invalid value '%s' at Line %d in '%s' file\n",
			     str2, idx + 2, in_file);
			goto err_exit;
		}

		if (!r->rdata)
			t = malloc(sizeof(struct reg_data));
		else
			t = realloc(r->rdata,
				    sizeof(struct reg_data) * (idx + 1));

		if (!t) {
			ERRS("Failed to increase memory for new reg\n");
			goto err_exit;
		}
		r->rdata = t;
		t[idx].addr = rdata.addr;
		t[idx].value = rdata.value;
		idx++;
	}
	if (!idx) {
		ERRF("invalid file format(No register data) in \"%s\"!\n",
		     in_file);
		goto err_exit;
	}

	/* Add null termination */
	t = realloc(r->rdata, sizeof(struct reg_data) * (idx + 1));

	if (!t) {
		ERRS("Failed to increase memory for new reg\n");
		goto err_exit;
	}
	r->rdata = t;
	t[idx].addr = 0;
	t[idx].value = 0;
	goto ok_out;

err_exit:
	if (r) {
		if (r->rdata)
			free(r->rdata);
		free(r);
		r = NULL;
	}

ok_out:
	if (in)
		fclose(in);

	return r;
}

/**
 * read_registers() - read the registers
 * @r:	actual rd1 input
 *
 * REMEMBER TO KEEP THIS AS OPTIMIZED AS SENSIBLE.
 */
static int read_registers(struct rd1 *r)
{
	unsigned int value;
	struct reg_data *rdata;
	int ret;

	if (!r)
		return -1;
	rdata = r->rdata;
	while (rdata && rdata->addr) {
		ret = readreg(rdata->addr, &value);
		if (ret)
			return ret;
		rdata->value = value;
		rdata++;
	}
	return 0;
}

/**
 * generate_output_file() - and generate the output file
 * @r:	register rd1 input
 * @out_file: if NULL, print to stdout, else pop it to file
 */
static int generate_output_file(struct rd1 *r, char *out_file)
{
	struct reg_data *rdata;
	FILE *out = stdout;
	int ret;

	if (out_file) {
		out = fopen(out_file, "w");
		if (!out) {
			ERRS("Unable to open \"%s\"\n", out_file);
			ret = -1;
			goto out;
		}
	}
	ret = fprintf(out, "%s\n", r->device_name);
	if (!ret || ret < 0) {
		ERRS("write failed=%d\n", ret);
		goto out;
	}

	rdata = r->rdata;
	while (rdata && rdata->addr) {
		ret =
		    fprintf(out, "0x%08x 0x%08x\n", rdata->addr, rdata->value);
		if (!ret || ret < 0) {
			ERRS("write failed=%d\n", ret);
			goto out;
		}
		rdata++;
	}
	ret = 0;

out:
	if (out_file && out)
		fclose(out);

	return ret;
}

int main(int argc, char **argv)
{
	int opt, r = 0;
	char *in_file = NULL;
	char *out_file = NULL;
	struct rd1 *device_rd1;

	app_name = argv[0];
	while ((opt = getopt(argc, argv, "i:o:")) != -1) {
		switch (opt) {
		case 'o':
			out_file = optarg;
			break;
		case 'i':
			in_file = optarg;
			break;
		default:
			usage("invalid option");
			/* DOES NOT RETURN */
		}
	}

	if (!in_file)
		usage("input_file missing (use -i)");

	device_rd1 = read_input_file(in_file);
	if (!device_rd1) {
		usage("Invalid File format!\n");
	}

	if (read_registers(device_rd1)) {
		ERRF("read memory failure!\n");
		r = EXIT_FAILURE;
		goto out;
	}

	r = generate_output_file(device_rd1, out_file);
out:
	if (device_rd1->rdata)
		free(device_rd1->rdata);
	free(device_rd1);

	return r;
}
