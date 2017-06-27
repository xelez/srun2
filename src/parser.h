/*
 *  Copyright 2013 Alexander Ankudinov
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#ifndef PARSER_H_
#define PARSER_H_

typedef enum parser_arg_t {
	PARSER_ARG_STR  = 1,
	PARSER_ARG_INT  = 2,
	PARSER_ARG_BOOL = 3
} parser_arg_t;

typedef struct parser_option_t {
	char *long_name;
	char *short_name;
	parser_arg_t type;
	void *variable;
	char *description;
} parser_option_t;

int parse_options(parser_option_t *options, int argc, char **argv);
void parser_print_help(parser_option_t *options);

#endif /* PARSER_H_ */
