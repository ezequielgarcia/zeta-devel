#!/usr/bin/perl

$whitelist{"IDirectFB"} 		= true;
$whitelist{"CreateSurface"}		= true;
$whitelist{"SetCooperativeLevel"} = true;
$whitelist{"IDirectFBSurface"} 	= true;
$whitelist{"Clear"} 			= true;
$whitelist{"Flip"} 				= true;
$whitelist{"FillRectangle"}		= true;
$whitelist{"SetColor"}			= true;
$whitelist{"StretchBlit"}		= true;

$gen_structs{"DFBRectangle"}			= true;
$gen_structs{"DFBSurfaceDescription"}	= true;
$gen_structs{"DFBRegion"}    			= true;

$src_dir = "./src/";

$pkgname = "directfb";

###############
## Utilities ##
###############

sub trim ($) {
	local (*str) = @_;

	# remove leading white space
	$str =~ s/^\s*//g;

	# remove trailing white space and new line
	$str =~ s/\s*$//g;
}

sub print_common_interface ($) {

	local ($interface) = @_;

	print COMMON_H  "DLL_LOCAL int open_${interface} (lua_State *L);\n";
	print COMMON_H  "DLL_LOCAL void push_${interface} (lua_State *L, ${interface} *interface);\n";
	print COMMON_C  "DLL_LOCAL void push_${interface} (lua_State *L, ${interface} *interface)\n",
					"{\n",
					"\t${interface} **p = lua_newuserdata(L, sizeof(${interface}*));\n",
					"\t*p = interface;\n",
					"\tluaL_getmetatable(L, \"${interface}\");\n",
					"\tlua_setmetatable(L, -2);\n",
					"}\n\n";

	print COMMON_H  "DLL_LOCAL ${interface} **check_${interface} (lua_State *L, int index);\n\n";
	print COMMON_C  "DLL_LOCAL ${interface} **check_${interface} (lua_State *L, int index)\n",
					"{\n",
					"\t${interface} **p;\n",
					"\tluaL_checktype(L, index, LUA_TUSERDATA);\n",
					"\tp = (${interface} **) luaL_checkudata(L, index, \"${interface}\");\n",
					"\tif (p == NULL) luaL_typerror(L, index, \"${interface}\");\n",
					"\treturn p;\n",
					"}\n\n";
}

###################
## File creation ##
###################

# TODO: If no-one uses includes, throw it away :)
sub h_create ($$$) {
	local ($FILE, $filename, $includes) = @_;

	open( $FILE, ">${src_dir}$filename" )
		or die ("*** Can not open '$filename' for writing:\n*** $!");

	print $FILE "#ifndef $FILE\n",
				"#define $FILE\n\n",
				"#include \"lua.h\"\n", 
				"#include \"lauxlib.h\"\n",
				"#include \"directfb.h\"\n",
				"\n",
				"$includes\n";
}

sub c_create ($$$) {
	local ($FILE, $filename, $includes) = @_;

	open( $FILE, ">${src_dir}$filename" )
		or die ("*** Can not open '$filename' for writing:\n*** $!");

	print $FILE "#include \"lua.h\"\n", 
				"#include \"lauxlib.h\"\n",
				"#include \"directfb.h\"\n",
				"\n",
				"$includes\n";
}

sub h_close ($) {
	local ($FILE) = @_;
	print $FILE "\n#endif\n";
	close( $FILE );
}

sub c_close ($) {
	local ($FILE) = @_;
	close( $FILE );
}

#############
## Parsing ##
#############

# Reads stdin until the end of the parameter list is reached.
# Returns list of parameter records.
#
# TODO: Add full comment support and use it for function types as well.
#
sub parse_params () {
	local @entries;

	while (<>) {
		chomp;
		last if /^\s*\)\;\s*$/;

		if ( /^\s*(const)?\s*([\w\ ]+)\s+(\**)(\w+),?\s*$/ ) {
			local $const = $1;
			local $type  = $2;
			local $ptr   = $3;
			local $name  = $4;

			trim( \$type );

			if (!($name eq "thiz")) {
				local $rec = {
					NAME   => $name,
					CONST  => $const,
					TYPE   => $type,
					PTR    => $ptr
				};

				push (@entries, $rec);
			}
		}
	}

	return @entries;
}

# Reads stdin until the end of the interface is reached.
# Parameter is the interface name.
#
sub parse_interface ($) {
	local ($interface) = @_;

	trim( \$interface );

	# DEBUG: ONLY
	return if (!($whitelist{$interface} eq true));

#	print("> Creating ${interface}.c\n");

	c_create( INTERFACE, "${interface}.c", "#include \"common.h\"\n#include \"structs.h\"\n" );

	local @funcs;

	while (<>) {
		chomp;
		last if /^\s*\)\s*$/;

		if ( /^\s*\/\*\*\s*(.+)\s*\*\*\/\s*$/ ) {
		}
		elsif ( /^\s*(\w+)\s*\(\s*\*\s*(\w+)\s*\)\s*\(?\s*$/ ) {
			next if (!($whitelist{$2} eq true));

			local $function   = $2;
			local $return_val = 0;

			local @params = parse_params();
			local $param;

#			print "  * New function: $function\n";

			local $args;
			local $declaration;
			local $pre_code;
			local $post_code;

			# Arg number starts at 2 (Lua starts at 1 plus self interface which uses 1).
			local $arg_num = 2;

			for $param (@params) {
#				print "   - New parameter: $param->{NAME}, $param->{CONST}, $param->{TYPE}, $param->{PTR} \n";

				#$pre_code .= "\n";

				# simple
				if ($param->{PTR} eq "") {
					$declaration .= "\t$param->{TYPE} $param->{NAME};\n";
					$pre_code .= "\t$param->{NAME} = luaL_checkinteger(L, $arg_num);\n";
					$args .= ", $param->{NAME}";
				}
				# pointer
				elsif ($param->{PTR} eq "*") {
					# input
					if ($param->{CONST} eq "const") {
						# "void" -> buffer input 
						if ($param->{TYPE} eq "void") {
							$pre_code .= "\t$param->{TYPE} *$param->{NAME};\n";
							$pre_code .= "\t#warning unimplemented (buffer input)\n";
							$pre_code .= "\tD_UNIMPLEMENTED();\n";

							$args .= ", $param->{NAME}";
						}
						# "char" -> string input
						elsif ($param->{TYPE} eq "char") {
							$declaration .= "\tconst $param->{TYPE} *$param->{NAME};\n";
							$pre_code .= "$param->{NAME} = luaL_checkstring(L, $arg_num);\n";
							$args .= ", $param->{NAME}";
						}
						# struct input, must handle nil value
						elsif ($types{$param->{TYPE}}->{KIND} eq "struct") {
							
							$declaration .= "\t$param->{TYPE} $param->{NAME}, *$param->{NAME}_p;\n";
							$pre_code .= "\t$param->{NAME}_p = check_$param->{TYPE}(L, $arg_num, &$param->{NAME});\n";
							$args .= ", $param->{NAME}_p";
						}
						# array input?
						else
						{
							$pre_code .= "     $param->{TYPE} $param->{NAME};\n";
							$pre_code .= "     #warning unimplemented (array input?)\n";
							$pre_code .= "     D_UNIMPLEMENTED();\n";

							$args .= ", &$param->{NAME}";
						}
					}
					
					# output
					else {
						# "void" -> just context pointer
						if ($param->{TYPE} eq "void") {
							$pre_code .= "     $param->{TYPE} *$param->{NAME};\n";
							$pre_code .= "     #warning unimplemented (context pointer)\n";
							$pre_code .= "     D_UNIMPLEMENTED();\n";

							$args .= ", $param->{NAME}";
						}
						# struct output
						elsif ($types{$param->{TYPE}}->{KIND} eq "struct") {
							$declaration .= "\t$param->{TYPE} $param->{NAME};\n";

							$args .= ", &$param->{NAME}";

							if ($return_val eq "v8::Undefined()") {
								$return_val = "$param->{TYPE}_construct( &$param->{NAME} )";
							}
							else {
								$post_code .= "     #warning unimplemented (second output)\n";
							}
						}
						# Interface input(!)
						elsif ($types{$param->{TYPE}}->{KIND} eq "interface") {
							$declaration .= "\t$param->{TYPE} **$param->{NAME};\n";
							$pre_code .= "\t$param->{NAME} = check_$param->{TYPE}(L, $arg_num);\n";
							$args .= ", *$param->{NAME}";
						}
						# enum? output
						else {
							$declaration .= "\t$param->{TYPE} $param->{NAME};\n";

							$args .= ", &$param->{NAME}";

							if ($return_val eq "v8::Undefined()") {
								$return_val = "v8::Integer::New( $param->{NAME} )";
							}
							else {
								$post_code .= "     #warning unimplemented (second output)\n";
							}
						}
					}
				}
				# double pointer
				elsif ($param->{PTR} eq "**") {
					# input (pass array)
					if ($param->{CONST} eq "const") {
						$pre_code .= "     $param->{TYPE} *$param->{NAME};\n";
						$pre_code .= "     #warning unimplemented (pointer array)\n";
						$pre_code .= "     D_UNIMPLEMENTED();\n";
					}
					# output (return interface)
					else {
						# "void" -> return buffer
						if ($param->{TYPE} eq "void") {
							$pre_code .= "     $param->{TYPE} *$param->{NAME};\n";
							$pre_code .= "     #warning unimplemented (return pointer)\n";
							$pre_code .= "     D_UNIMPLEMENTED();\n";
						}
						# "char" -> return string
						elsif ($param->{TYPE} eq "char") {
							$pre_code .= "     $param->{TYPE} *$param->{NAME};\n";
							$pre_code .= "     #warning unimplemented (return string)\n";
							$pre_code .= "     D_UNIMPLEMENTED();\n";
						}
						# output (return interface)
						else {
							$declaration .= "\t$param->{TYPE} *$param->{NAME};\n";
							$post_code .= "\tpush_$param->{TYPE}(L, $param->{NAME});\n";
							$return_val++;
						}
					}

					$args .= ", &$param->{NAME}";
				}

				$arg_num++;
			}

			# Append new line in front of post_code for cleanear code, you obsessive bastard.
			if ($post_code ne "") {
				$post_code = "\n".$post_code;
			}

			print INTERFACE "static int\n";
			print INTERFACE "l_${interface}_${function} (lua_State *L)\n";
			print INTERFACE "{\n";
			print INTERFACE "\t${interface} **thiz;\n";
			print INTERFACE "${declaration}\n";
		   	print INTERFACE "\tthiz = check_${interface}(L, 1);\n";
			print INTERFACE "${pre_code}\n";
			print INTERFACE "\t(*thiz)->${function}( *thiz${args} );\n";
			print INTERFACE "${post_code}\n";
			print INTERFACE "\treturn ${return_val};\n";
			print INTERFACE "}\n";
			print INTERFACE "\n";

			push( @funcs, {
					NAME   => $function,
					PARAMS => @params
					} );
		}
		elsif ( /^\s*\/\*\s*$/ ) {
		#	parse_comment( \$headline, \$detailed, \$options, "" );
		}
	}

	print INTERFACE "\n";

	print INTERFACE "static const luaL_reg ${interface}_methods[] = {\n";

	for $func (@funcs) {
		print INTERFACE "\t{\"$func->{NAME}\",l_${interface}_$func->{NAME}},\n";
	}

	print INTERFACE "\t{NULL, NULL}\n",
					"};\n\n";

	print INTERFACE "DLL_LOCAL int open_${interface} (lua_State *L)\n",
					"{\n",
					"\tluaL_newmetatable(L, \"${interface}\");\n",
					"\tlua_pushstring(L, \"__index\");\n",
					"\tlua_pushvalue(L, -2);\n",
					"\tlua_settable(L, -3);\n",
					"\tluaL_openlib(L, NULL, ${interface}_methods, 0);\n",
					"\treturn 1;\n",
					"}\n";

	c_close( INTERFACE );
}

# Reads stdin until the end of the enum is reached.
#
sub parse_enum {
	local @entries;

	while (<>) {
		chomp;

		local $entry;

		# entry with assignment (complete comment)
		if ( /^\s*(\w+)\s*=\s*([\w\d\(\)\,\|\!\s]+[^\,\s])\s*,?\s*\/\*\s*(.+)\s*\*\/\s*$/ ) {
			$entry = $1;
		}
		# entry with assignment (opening comment)
		elsif ( /^\s*(\w+)\s*=\s*([\w\d\(\)\,\|\!\s]+[^\,\s])\s*,?\s*\/\*\s*(.+)\s*$/ ) {
			$entry = $1;
		}
		# entry with assignment (none or preceding comment)
		elsif ( /^\s*(\w+)\s*=\s*([\w\d\(\)\,\|\!\s]+[^\,\s])\s*,?\s*$/ ) {
			$entry = $1;
		}
		# entry without assignment (complete comment)
		elsif ( /^\s*(\w+)\s*,?\s*\/\*\s*(.+)\s*\*\/\s*$/ ) {
			$entry = $1;
		}
		# entry without assignment (opening comment)
		elsif ( /^\s*(\w+)\s*,?\s*\/\*\s*(.+)\s*$/ ) {
			$entry = $1;
		}
		# entry without assignment (none or preceding comment)
		elsif ( /^\s*(\w+)\s*,?\s*$/ ) {
			$entry = $1;
		}
		# preceding comment (complete)
		elsif ( /^\s*\/\*\s*(.+)\s*\*\/\s*$/ ) {
		}
		# preceding comment (opening)
		elsif ( /^\s*\/\*\s*(.+)\s*$/ ) {
		}
		# end of enum
		elsif ( /^\s*\}\s*(\w+)\s*\;\s*$/ ) {
			$enum = $1;
			last;
		}
		# blank line?
		else {
		}

		if ($entry ne "") {
			push (@entries, $entry);
		
			# Map this entry to a global variable. 
			# Won't have any type checking from the lua side,
			# as it is only a number variable.
			print ENUMS_C "\tlua_pushnumber(L, $entry);\n";
			print ENUMS_C "\tlua_setglobal(L, \"$entry\");\n\n";
		}
	}

	$types{$enum} = {
		NAME    => $enum,
		KIND    => "enum",
		ENTRIES => @entries
	};
}

# Reads stdin until the end of the enum is reached.
#
sub parse_struct {
	local @entries;

	while (<>) {
		chomp;

		local $entry;

		# without comment
		if ( /^\s*(const )?\s*([\w ]+)\s+(\**)([\w\d\+\[\]]+)(\s*:\s*\d+)?;\s*$/ ) {
			$const = $1;
			$type = $2;
			$ptr = $3;
			$entry = $4.$5;
			$text = "";
		}
		# complete one line entry
		elsif ( /^\s*(const )?\s*([\w ]+)\s+(\**)([\w\d\+\[\]]+)(\s*:\s*\d+)?;\s*\/\*\s*(.+)\*\/\s*$/ ) {
			$const = $1;
			$type = $2;
			$ptr = $3;
			$entry = $4.$5;
			$text = $6;
		}
		# with comment opening
		elsif ( /^\s*(const )?\s*([\w ]+)\s+(\**)([\w\d\+\[\]]+)(\s*:\s*\d+)?;\s*\/\*\s*(.+)\s*$/ ) {
			$const = $1;
			$type = $2;
			$ptr = $3;
			$entry = $4.$5;

			$text = $t1.$t2;
		}
		# sub
		elsif ( /^\s*struct\s*\{\s*$/ ) {
			while (<>) {
				chomp;
				last if /^\s*\}\s*([\w\d\+\[\]]+)\s*\;\s*/;
			}
		}
		elsif ( /^\s*\}\s*(\w+)\s*\;\s*$/ ) {
			$struct = $1;

			trim( \$struct );

			$struct_list{$struct} = $headline;
			$type_list{$struct} = $headline;

			last;
		}

		trim( \$type );

		if ($entry ne "") {
			push (@entries, {
					NAME   => $entry,
					CONST  => $const,
					TYPE   => $type,
					PTR    => $ptr
					} );
		}
	}

	$types{$struct} = {
		NAME    => $struct,
		KIND    => "struct",
		ENTRIES => @entries
	};

#	print "New struct: $struct\n";

#	# header
#	print TEMPLATES_H "extern v8::Handle<v8::Object> ${struct}_construct( const ${struct} *src );\n";
#	print TEMPLATES_H "extern void ${struct}_read( ${struct} *dst, v8::Handle<v8::Value> src );\n";
#
#	# _construct
#	print TEMPLATES_CC "\n",
#		  "v8::Handle<v8::Object>\n",
#		  "${struct}_construct( const ${struct} *src )\n",
#		  "{\n",
#		  "     v8::Handle<v8::Object> obj = v8::Object::New();\n",
#		  "\n";
#
#	if ($gen_structs{$struct}) {
#		foreach $entry (@entries) {
#			if ($types{$entry->{TYPE}}->{KIND} eq "struct") {
#				print TEMPLATES_CC "     obj->Set( v8::String::NewSymbol( \"$entry->{NAME}\" ), $entry->{TYPE}_construct( &src->$entry->{NAME} ) );\n";
#			}
#			else {
#				print TEMPLATES_CC "     obj->Set( v8::String::NewSymbol( \"$entry->{NAME}\" ), v8::Integer::New(src->$entry->{NAME}) );\n";
#			}
#		}
#	}
#
#	print TEMPLATES_CC "\n";
#	print TEMPLATES_CC "     return obj;\n";
#	print TEMPLATES_CC "}\n";

	if ($gen_structs{$struct}) {
		print STRUCTS_H "DLL_LOCAL ${struct}* check_${struct} (lua_State *L, int index, ${struct} *dst);\n";
		print STRUCTS_C "DLL_LOCAL ${struct}* check_${struct} (lua_State *L, int index, ${struct} *dst)\n",
						"{\n",
						"\tif (lua_isnil(L, index)) \n",
						"\t\treturn NULL;\n\n",
						"\tluaL_checktype(L, index, LUA_TTABLE);\n",
		  				"\tmemset(dst, 0, sizeof(${struct}));\n";
	# _read
#	print STRUCTS_C "\n",
#		  "void\n",
#		  "${struct}_read( ${struct} *dst, v8::Handle<v8::Value> src )\n",
#		  "{\n",
#		  "     if (src.IsEmpty()) { memset( dst, 0, sizeof(${struct}) ); return; }\n",
#		  "\n",
#		  "     v8::Handle<v8::Object> obj = src->ToObject();\n",
#		  "\n";

		foreach $entry (@entries) {
			if ($types{$entry->{TYPE}}->{KIND} eq "struct") {
				print STRUCTS_C "\n\tcheck_$entry->{TYPE}(L, ?, &dst->$entry->{NAME});\n";
			}
			else {
				print STRUCTS_C "\n\tlua_getfield(L, index, \"$entry->{NAME}\");\n";
				print STRUCTS_C "\tdst->$entry->{NAME} = lua_tonumber(L, -1);\n";
				print STRUCTS_C "\tlua_pop(L, 1);\n";
			}
		}

		print STRUCTS_C "\t\n\treturn dst;\n";
		print STRUCTS_C "}\n\n";
	}
}

#
# Reads stdin until the end of the function type is reached.
# Parameters are the return type and function type name.
#
sub parse_func ($$) {
	local ($rtype, $name) = @_;

	local @entries;
	local %entries_params;
	local %entries_types;
	local %entries_ptrs;

	trim( \$rtype );
	trim( \$name );

	while (<>) {
		chomp;

		local $entry;

		# without comment
		if ( /^\s*(const )?\s*([\w ]+)\s+(\**)([\w\d\+\[\]]+)(\s*:\s*\d+)?,?\s*$/ ) {
			$const = $1;
			$type = $2;
			$ptr = $3;
			$entry = $4.$5;
			$text = "";
		}
		# complete one line entry
		elsif ( /^\s*(const )?\s*([\w ]+)\s+(\**)([\w\d\+\[\]]+)(\s*:\s*\d+)?,?\s*\/\*\s*(.+)\*\/\s*$/ ) {
			$const = $1;
			$type = $2;
			$ptr = $3;
			$entry = $4.$5;
			$text = $6;
		}
		# with comment opening
		elsif ( /^\s*(const )?\s*([\w ]+)\s+(\**)([\w\d\+\[\]]+)(\s*:\s*\d+)?,?\s*\/\*\s*(.+)\s*$/ ) {
			$const = $1;
			$type = $2;
			$ptr = $3;
			$entry = $4.$5;
			$text = $t1.$t2;
		}
		elsif ( /^\s*\)\;\s*$/ ) {
			$func_list{$name} = $headline;
			$type_list{$name} = $headline;

			last;
		}

		if ($entry ne "") {
			# TODO: Use structure
			$entries_types{$entry} = $const . $type;
			$entries_ptrs{$entry} = $ptr;
			$entries_params{$entry} = $text;

			push (@entries, $entry);

		}
	}
}

##########
## Main ##
##########

h_create( COMMON_H, "common.h", "" );
c_create( COMMON_C, "common.c", "#include \"common.h\"\n" );

h_create( STRUCTS_H, "structs.h", "" );
c_create( STRUCTS_C, "structs.c", "#include \"common.h\"\n" );

c_create( ENUMS_C, "enums.c", "#include \"common.h\"\n" );

print COMMON_H 	"#if defined(__GNUC__) && __GNUC__ >= 4\n",
				"\t#define DLL_EXPORT __attribute__((visibility(\"default\")))\n",
				"\t#define DLL_LOCAL	__attribute__((visibility(\"hidden\")))\n",
				"#else\n",
				"\t#define DLL_EXPORT\n",
				"\t#define DLL_LOCAL\n",
				"#endif\n\n",
				"DLL_LOCAL void open_enums (lua_State *L);\n";

# TODO: Global variables or .. ?
# Start open_enum function. This maps enum symbols to lua global variables.
print ENUMS_C 	"DLL_LOCAL void open_enums (lua_State *L)\n",
			   	"{\n";
while (<>) {
	chomp;

	# Search interface declaration
	if ( /^\s*DECLARE_INTERFACE\s*\(\s*(\w+)\s\)\s*$/ ) {
		$interface = $1;

		trim( \$interface );

		next if (!($whitelist{$interface} eq true));
		
		print_common_interface($interface);

		if (!defined ($types{$interface})) {
			$types{$interface} = {
				NAME    => $interface,
				KIND    => "interface"
			};
		}
	}
	elsif ( /^\s*DEFINE_INTERFACE\s*\(\s*(\w+),\s*$/ ) {
		parse_interface( $1 );
	}
	elsif ( /^\s*typedef\s+enum\s*\{?\s*$/ ) {
		parse_enum();
	}
	elsif ( /^\s*typedef\s+(struct|union)\s*\{?\s*$/ ) {
		parse_struct();
	}
	elsif ( /^\s*typedef\s+(\w+)\s+\(\*(\w+)\)\s*\(\s*$/ ) {
		parse_func( $1, $2 );
	}
	elsif ( /^\s*#define\s+([^\(\s]+)(\([^\)]*\))?\s*(.*)/ ) {
#		parse_macro( $1, $2, $3 );
	}
	elsif ( /^\s*\/\*\s*$/ ) {
#		parse_comment( \$headline, \$detailed, \$options, "" );
	}
	else {
		$headline = "";
		$detailed = "";
		%options  = ();
	}
}

# End enum function
print ENUMS_C 	"}\n";

# Library initialization code
print COMMON_C 	"static int l_DirectFBInit (lua_State *L)\n",
			   	"{\n",
				"\tDirectFBInit(NULL, NULL);\n",
				"\treturn 0;\n",
				"}\n\n";

print COMMON_C	"static int l_DirectFBCreate (lua_State *L)\n",
				"{\n",
				"\tIDirectFB *interface;\n",
				"\tDirectFBCreate(&interface);\n",
				"\tpush_IDirectFB(L, interface);\n",
				"\treturn 1;\n",
				"}\n\n";

print COMMON_C  "static const luaL_reg dfb_m[] = {\n",
				"\t{\"DirectFBCreate\", l_DirectFBCreate},\n",
				"\t{\"DirectFBInit\", l_DirectFBInit},\n",
				"\t{NULL, NULL}\n",
				"};\n\n";

print COMMON_C "int LUALIB_API luaopen_$pkgname (lua_State *L)\n",
	 		   "{\n";

my @interfaces = grep { $types{$_}{KIND} eq "interface" } keys %types;
foreach $name (@interfaces) {
	print COMMON_C "\topen_$name(L);\n",
}

print COMMON_C "\n",
			   "\tluaL_openlib(L, \"$pkgname\", dfb_m, 0);\n",
				"\topen_enums(L);\n",
			   "\treturn 1;\n",
			   "}";

h_close( COMMON_H );
c_close( COMMON_C );

h_close( STRUCTS_H );
c_close( STRUCTS_C );

c_close( ENUMS_C );
