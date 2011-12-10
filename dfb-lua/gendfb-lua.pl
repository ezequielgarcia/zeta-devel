#!/usr/bin/perl

$whitelist{"IDirectFBSurface"} = true;
$whitelist{"Clear"} = true;

$src_dir = ".src/";

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

###################
## File creation ##
###################

# TODO: If no-one uses includes, throw it away :)
sub src_create ($$$) {
	local ($FILE, $filename, $includes) = @_;

	open( $FILE, ">$filename" )
		or die ("*** Can not open '$filename' for writing:\n*** $!");

	print $FILE "#include \"lua.h\"\n", 
				"#include \"lauxlib.h\"\n",
				"#include \"core.h\"\n",
				"#include \"directfb.h\"\n",
				"\n",
				"$includes\n";
}

sub src_close ($) {
	local ($FILE) = @_;

	print $FILE "\n";

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

	print("Creating ${interface}.cc\n");

	src_create( INTERFACE, "${src_dir}${interface}.cc", "" );

	local @funcs;

	while (<>) {
		chomp;
		last if /^\s*\)\s*$/;

		if ( /^\s*\/\*\*\s*(.+)\s*\*\*\/\s*$/ ) {
		}
		elsif ( /^\s*(\w+)\s*\(\s*\*\s*(\w+)\s*\)\s*\(?\s*$/ ) {
			next if (!($whitelist{$2} eq true));

			local $function   = $2;
			local $return_val = "0";

			local @params = parse_params();
			local $param;

			print "New function: $function\n";

			local $args;
			local $pre_code;
			local $post_code;

			local $arg_num = 0;

			for $param (@params) {
				print "New parameter: $param->{NAME}, $param->{CONST}, $param->{TYPE}, $param->{PTR} \n";

				$pre_code .= "\n";

				# simple
				if ($param->{PTR} eq "") {
					$pre_code .= "\t$param->{TYPE} $param->{NAME} = luaL_checkinteger(L, $arg_num);\n";

					$args .= ", $param->{NAME}";
				}
				# pointer
				elsif ($param->{PTR} eq "*") {
					# input
					if ($param->{CONST} eq "const") {
						# "void" -> buffer input 
						if ($param->{TYPE} eq "void") {
							$pre_code .= "     $param->{TYPE} *$param->{NAME};\n";
							$pre_code .= "     #warning unimplemented (buffer input)\n";
							$pre_code .= "     D_UNIMPLEMENTED();\n";

							$args .= ", $param->{NAME}";
						}
						# "char" -> string input
						elsif ($param->{TYPE} eq "char") {
							$pre_code .= "     const $param->{TYPE} *$param->{NAME} = NULL;\n";
							$pre_code .= "     v8::String::Utf8Value utf8_$param->{NAME}(args[$arg_num]);\n";
							$pre_code .= "     $param->{NAME} = *utf8_$param->{NAME};\n";

							$args .= ", $param->{NAME}";
						}
						# struct input
						elsif ($types{$param->{TYPE}}->{KIND} eq "struct") {
							$pre_code .= "     $param->{TYPE} *_$param->{NAME} = NULL;\n";
							$pre_code .= "     $param->{TYPE} $param->{NAME};\n";
							$pre_code .= "     if (args[$arg_num]->IsObject()) {\n";
							$pre_code .= "          $param->{TYPE}_read( &$param->{NAME}, args[$arg_num] );\n";
							$pre_code .= "          _$param->{NAME} = &$param->{NAME};\n";
							$pre_code .= "     }\n";

							$args .= ", _$param->{NAME}";
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
							$pre_code .= "     $param->{TYPE} $param->{NAME};\n";

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
							$pre_code .= "     $param->{TYPE} *$param->{NAME} = V8_DIRECTFB_INTERFACE( args[$arg_num]->ToObject(), $param->{TYPE} );\n";

							$args .= ", $param->{NAME}";
						}
						# enum? output
						else {
							$pre_code .= "     $param->{TYPE} $param->{NAME};\n";

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
							$pre_code .= "     $param->{TYPE} *$param->{NAME};\n";

							$post_code .= "     v8::Handle<v8::ObjectTemplate> templ = $param->{TYPE}_template();\n";

							$return_val = "Construct( templ, $param->{NAME} )";
						}
					}

					$args .= ", &$param->{NAME}";
				}

				$arg_num++;
			}

			print INTERFACE "static int\n";
			print INTERFACE "l_${interface}_${function} (lua_State *L)\n";
			print INTERFACE "{\n";
			print INTERFACE "\t${interface} **thiz = check_${interface}(L, 1);\n";
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
		print INTERFACE "\t{\"$func->{NAME}\"},l_${interface}_$func->{NAME},\n";
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

	src_close( INTERFACE );
}

##########
## Main ##
##########

while (<>) {
	chomp;

# Search interface declaration
	if ( /^\s*DECLARE_INTERFACE\s*\(\s*(\w+)\s\)\s*$/ ) {
		$interface = $1;

		trim( \$interface );

		print("New interface declaration: $interface\n");

#		if (!defined ($types{$interface})) {
#			$types{$interface} = {
#				NAME    => $interface,
#				KIND    => "interface"
#			};
#
#
#			print TEMPLATES_H "extern v8::Handle<v8::ObjectTemplate> ${interface}_template();\n";
#		}
	}
	elsif ( /^\s*DEFINE_INTERFACE\s*\(\s*(\w+),\s*$/ ) {
		print("New interface definition: $1\n");
		parse_interface( $1 );
	}
	elsif ( /^\s*typedef\s+enum\s*\{?\s*$/ ) {
#		parse_enum();
	}
	elsif ( /^\s*typedef\s+(struct|union)\s*\{?\s*$/ ) {
#		parse_struct();
	}
	elsif ( /^\s*typedef\s+(\w+)\s+\(\*(\w+)\)\s*\(\s*$/ ) {
#		parse_func( $1, $2 );
	}
	elsif ( /^\s*#define\s+([^\(\s]+)(\([^\)]*\))?\s*(.*)/ ) {
#		parse_macro( $1, $2, $3 );
	}
	elsif ( /^\s*\/\*\s*$/ ) {
#		parse_comment( \$headline, \$detailed, \$options, "" );
	}
	else {
#		$headline = "";
#		$detailed = "";
#		%options  = ();
	}
}
