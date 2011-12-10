#!/usr/bin/perl
#
#   (c) Copyright 2010  The DirectFB Organization (directfb.org)
#
#   All rights reserved.
#
#   Written by Denis Oliver Kropp <dok@directfb.org>
#
#   This library is free software; you can redistribute it and/or
#   modify it under the terms of the GNU Lesser General Public
#   License as published by the Free Software Foundation; either
#   version 2 of the License, or (at your option) any later version.
#
#   This library is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#   Lesser General Public License for more details.
#
#   You should have received a copy of the GNU Lesser General Public
#   License along with this library; if not, write to the
#   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
#   Boston, MA 02111-1307, USA.
#
#
#   Based on gendoc.pl, still containing obsolete pieces of that...
# 

$blacklist{"GetStringBreak"}        = true;
$blacklist{"SetStreamAttributes"}   = true;
$blacklist{"SetBufferThresholds"}   = true;
$blacklist{"SetClipboardData"}      = true;
$blacklist{"GetClipboardTimeStamp"} = true;

$gen_structs{"DFBColor"}              = true;
$gen_structs{"DFBEvent"}              = true;
$gen_structs{"DFBFontDescription"}    = true;
$gen_structs{"DFBRectangle"}          = true;
$gen_structs{"DFBRegion"}             = true;
$gen_structs{"DFBSurfaceDescription"} = true;
$gen_structs{"DFBWindowDescription"}  = true;

######################################################################################
#                                                                                    #
#  JavScript (V8) binding generator written by Denis Oliver Kropp <dok@directfb.org> #
#                                                                                    #
#  - Uses first argument as project name and second as version                       #
#  - Reads header files from stdin, parsing is tied to the coding style              #
#  - Writes HTML 3.x to different files: 'index', 'types', <interfaces>, <methods>   #
#                                                                                    #
#  FIXME: remove all copy'n'waste code, cleanup more, simplify more, ...             #
#                                                                                    #
######################################################################################


########################################################################################################################
## Top level just calls main function with args
#

$PROJECT = shift @ARGV;
$VERSION = shift @ARGV;

gen_doc( $PROJECT, $VERSION );

########################################################################################################################

########################################################################################################################
## Utilities
#

sub trim ($) {
   local (*str) = @_;

   # remove leading white space
   $str =~ s/^\s*//g;

   # remove trailing white space and new line
   $str =~ s/\s*$//g;
}

########################################################################################################################
## Generic parsers
#

sub parse_comment ($$$$) {
   local (*head, *body, *options, $inithead) = @_;

   local $headline_mode = 1;
   local $list_open     = 0;

   $head = "\n";
   $body = "\n";

   if ($inithead ne "") {
      $headline_mode = 0;

      $head .= "        $inithead\n";
   }

   %options = ();

   while (<>)
      {
         chomp;
         last if /^\s*\*+\/\s*$/;

         # Prepend asterisk if first non-whitespace isn't an asterisk
         s/^\s*([^\*\s])/\* $1/;

         # In head line mode append to $head
         if ($headline_mode == 1)
            {
               if (/^\s*\*+\s*$/)
                  {
                     $headline_mode = 0;
                  }
               elsif (/^\s*\*+\s*@(\w+)\s*=?\s*(.*)$/)
                  {
                     $options{$1} = $2;
                  }
               elsif (/^\s*\*+\s*(.+)\*\/\s*$/)
                  {
                     $head .= "        $1\n";
                     last;
                  }
               elsif (/^\s*\*+\s*(.+)$/)
                  {
                     $head .= "        $1\n";
                  }
            }
         else
            # Otherwise append to $body
            {
               if (/^\s*\*+\s*(.+)\*\/\s*$/)
                  {
                     $body .= "        $1\n";
                     last;
                  }
               elsif (/^\s*\*+\s*$/)
                  {
                     $body .= " </P><P>\n";
                  }
               elsif (/^\s*\*+\s\-\s(.+)$/)
                  {
                     if ($list_open == 0)
                        {
                           $list_open = 1;

                           $body .= " <UL><LI>\n";
                        }
                     else
                        {
                           $body .= " </LI><LI>\n";
                        }

                     $body .= "        $1\n";
                  }
               elsif (/^\s*\*+\s\s(.+)$/)
                  {
                     $body .= "        $1\n";
                  }
               elsif (/^\s*\*+\s(.+)$/)
                  {
                     if ($list_open == 1)
                        {
                           $list_open = 0;

                           $body .= " </LI></UL>\n";
                        }

                     $body .= "        $1\n";
                  }
            }
      }

   if ($list_open == 1)
      {
         $body .= " </LI></UL>\n";
      }
}

#
# Reads stdin until the end of the parameter list is reached.
# Returns list of parameter records.
#
# TODO: Add full comment support and use it for function types as well.
#
sub parse_params () {
   local @entries;

   while (<>)
      {
         chomp;
         last if /^\s*\)\;\s*$/;

         if ( /^\s*(const)?\s*([\w\ ]+)\s+(\**)(\w+),?\s*$/ )
            {
               local $const = $1;
               local $type  = $2;
               local $ptr   = $3;
               local $name  = $4;

               trim( \$type );

               if (!($name eq "thiz"))
                  {
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

########################################################################################################################
## Type parsers
#

#
# Reads stdin until the end of the interface is reached.
# Writes formatted HTML to one file for the interface and one file per method.
# Parameter is the interface name.
#
sub parse_interface ($)
   {
      local ($interface) = @_;

      trim( \$interface );

      cc_create( INTERFACE, "${interface}.cc", "#include \"templates.h\"\n" );

      print INTERFACE "static v8::Persistent<v8::ObjectTemplate>  m_${interface};\n";
      print INTERFACE "\n";
      print INTERFACE "/**********************************************************************************************************************/\n";
      print INTERFACE "\n";


      local @funcs;

      while (<>)
         {
            chomp;
            last if /^\s*\)\s*$/;

            if ( /^\s*\/\*\*\s*(.+)\s*\*\*\/\s*$/ )
               {
               }
            elsif ( /^\s*(\w+)\s*\(\s*\*\s*(\w+)\s*\)\s*\(?\s*$/ )
               {
                  next if ($blacklist{$2} eq true);

                  local $function   = $2;
                  local $return_val = "v8::Undefined()";

                  local @params = parse_params();
                  local $param;

                  local $args;
                  local $pre_code;
                  local $post_code;

                  local $arg_num = 0;

                  for $param (@params)
                     {
                        $pre_code .= "\n";

                        # simple
                        if ($param->{PTR} eq "")
                           {
                              $pre_code .= "     $param->{TYPE} $param->{NAME} = ($param->{TYPE}) args[$arg_num]->IntegerValue();\n";

                              $args .= ", $param->{NAME}";
                           }
                        # pointer
                        elsif ($param->{PTR} eq "*")
                           {
                              # input
                              if ($param->{CONST} eq "const")
                                 {
                                    # "void" -> buffer input
                                    if ($param->{TYPE} eq "void")
                                       {
                                          $pre_code .= "     $param->{TYPE} *$param->{NAME};\n";
                                          $pre_code .= "     #warning unimplemented (buffer input)\n";
                                          $pre_code .= "     D_UNIMPLEMENTED();\n";

                                          $args .= ", $param->{NAME}";
                                       }
                                    # "char" -> string input
                                    elsif ($param->{TYPE} eq "char")
                                       {
                                          $pre_code .= "     const $param->{TYPE} *$param->{NAME} = NULL;\n";
                                          $pre_code .= "     v8::String::Utf8Value utf8_$param->{NAME}(args[$arg_num]);\n";
                                          $pre_code .= "     $param->{NAME} = *utf8_$param->{NAME};\n";

                                          $args .= ", $param->{NAME}";
                                       }
                                    # struct input
                                    elsif ($types{$param->{TYPE}}->{KIND} eq "struct")
                                       {
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
                              else
                                 {
                                    # "void" -> just context pointer
                                    if ($param->{TYPE} eq "void")
                                       {
                                          $pre_code .= "     $param->{TYPE} *$param->{NAME};\n";
                                          $pre_code .= "     #warning unimplemented (context pointer)\n";
                                          $pre_code .= "     D_UNIMPLEMENTED();\n";

                                          $args .= ", $param->{NAME}";
                                       }
                                    # struct output
                                    elsif ($types{$param->{TYPE}}->{KIND} eq "struct")
                                       {
                                          $pre_code .= "     $param->{TYPE} $param->{NAME};\n";

                                          $args .= ", &$param->{NAME}";

                                          if ($return_val eq "v8::Undefined()")
                                             {
                                                $return_val = "$param->{TYPE}_construct( &$param->{NAME} )";
                                             }
                                          else
                                             {
                                                $post_code .= "     #warning unimplemented (second output)\n";
                                             }
                                       }
                                    # Interface input(!)
                                    elsif ($types{$param->{TYPE}}->{KIND} eq "interface")
                                       {
                                          $pre_code .= "     $param->{TYPE} *$param->{NAME} = V8_DIRECTFB_INTERFACE( args[$arg_num]->ToObject(), $param->{TYPE} );\n";

                                          $args .= ", $param->{NAME}";
                                       }
                                    # enum? output
                                    else
                                       {
                                          $pre_code .= "     $param->{TYPE} $param->{NAME};\n";

                                          $args .= ", &$param->{NAME}";

                                          if ($return_val eq "v8::Undefined()")
                                             {
                                                $return_val = "v8::Integer::New( $param->{NAME} )";
                                             }
                                          else
                                             {
                                                $post_code .= "     #warning unimplemented (second output)\n";
                                             }
                                       }
                                 }
                           }
                        # double pointer
                        elsif ($param->{PTR} eq "**")
                           {
                              # input (pass array)
                              if ($param->{CONST} eq "const")
                                 {
                                    $pre_code .= "     $param->{TYPE} *$param->{NAME};\n";
                                    $pre_code .= "     #warning unimplemented (pointer array)\n";
                                    $pre_code .= "     D_UNIMPLEMENTED();\n";
                                 }
                              # output (return interface)
                              else
                                 {
                                    # "void" -> return buffer
                                    if ($param->{TYPE} eq "void")
                                       {
                                          $pre_code .= "     $param->{TYPE} *$param->{NAME};\n";
                                          $pre_code .= "     #warning unimplemented (return pointer)\n";
                                          $pre_code .= "     D_UNIMPLEMENTED();\n";
                                       }
                                    # "char" -> return string
                                    elsif ($param->{TYPE} eq "char")
                                       {
                                          $pre_code .= "     $param->{TYPE} *$param->{NAME};\n";
                                          $pre_code .= "     #warning unimplemented (return string)\n";
                                          $pre_code .= "     D_UNIMPLEMENTED();\n";
                                       }
                                    # output (return interface)
                                    else
                                       {
                                          $pre_code .= "     $param->{TYPE} *$param->{NAME};\n";

                                          $post_code .= "     v8::Handle<v8::ObjectTemplate> templ = $param->{TYPE}_template();\n";
      
                                          $return_val = "Construct( templ, $param->{NAME} )";
                                       }
                                 }

                              $args .= ", &$param->{NAME}";
                           }

                        $arg_num++;
                     }


                  print INTERFACE "static v8::Handle<v8::Value>\n";
                  print INTERFACE "${interface}_${function}( const v8::Arguments& args )\n";
                  print INTERFACE "{\n";
                  print INTERFACE "     ${interface} *thiz = V8_DIRECTFB_INTERFACE( args.This(), ${interface} );\n";
                  print INTERFACE "\n";
                  print INTERFACE "     if (!thiz)\n";
                  print INTERFACE "          return v8::ThrowException( v8::String::New( \"Interface already released\" ) );\n";
                  print INTERFACE "\n";
                  print INTERFACE "${pre_code}\n";
                  print INTERFACE "     V8_DIRECTFB_CALL( thiz->${function}( thiz${args} ) );\n";
                  print INTERFACE "${post_code}\n";
                  print INTERFACE "     return ${return_val};\n";
                  print INTERFACE "}\n";
                  print INTERFACE "\n";


                  push( @funcs, {
                     NAME   => $function,
                     PARAMS => @params
                  } );
               }
            elsif ( /^\s*\/\*\s*$/ )
               {
                  parse_comment( \$headline, \$detailed, \$options, "" );
               }
         }

      print INTERFACE "/**********************************************************************************************************************/\n";
      print INTERFACE "\n";

      print INTERFACE "v8::Handle<v8::ObjectTemplate>\n";
      print INTERFACE "${interface}_template()\n";
      print INTERFACE "{\n";
      print INTERFACE "     if (m_${interface}.IsEmpty()) {\n";
      print INTERFACE "          v8::HandleScope handle_scope;\n";
      print INTERFACE "          v8::Handle<v8::ObjectTemplate> templ;\n";
      print INTERFACE "\n";
      print INTERFACE "          templ = v8::ObjectTemplate::New();\n";
      print INTERFACE "\n";
      print INTERFACE "          templ->SetInternalFieldCount( 1 );\n";
      print INTERFACE "\n";

      print INTERFACE "          templ->Set( v8::String::NewSymbol(\"Release\"), v8::FunctionTemplate::New(IAny_release) );\n";

      for $func (@funcs)
         {
            print INTERFACE "          templ->Set( v8::String::NewSymbol(\"$func->{NAME}\"), v8::FunctionTemplate::New(${interface}_$func->{NAME}) );\n";
         }

      print INTERFACE "\n";
      print INTERFACE "          m_${interface} = v8::Persistent<v8::ObjectTemplate>::New( handle_scope.Close( templ ) );\n";
      print INTERFACE "     }\n";
      print INTERFACE "\n";
      print INTERFACE "     return m_${interface};\n";
      print INTERFACE "}\n";

      cc_close( INTERFACE );
   }

#
# Reads stdin until the end of the enum is reached.
# Writes formatted HTML to "types.cc".
#
sub parse_enum
   {
      local @entries;

      while (<>)
         {
            chomp;

            local $entry;

            # entry with assignment (complete comment)
            if ( /^\s*(\w+)\s*=\s*([\w\d\(\)\,\|\!\s]+[^\,\s])\s*,?\s*\/\*\s*(.+)\s*\*\/\s*$/ )
               {
                  $entry = $1;
               }
            # entry with assignment (opening comment)
            elsif ( /^\s*(\w+)\s*=\s*([\w\d\(\)\,\|\!\s]+[^\,\s])\s*,?\s*\/\*\s*(.+)\s*$/ )
               {
                  $entry = $1;

                  parse_comment( \$t1, \$t2, \$opt, $3 );
               }
            # entry with assignment (none or preceding comment)
            elsif ( /^\s*(\w+)\s*=\s*([\w\d\(\)\,\|\!\s]+[^\,\s])\s*,?\s*$/ )
               {
                  $entry = $1;
               }
            # entry without assignment (complete comment)
            elsif ( /^\s*(\w+)\s*,?\s*\/\*\s*(.+)\s*\*\/\s*$/ )
               {
                  $entry = $1;
               }
            # entry without assignment (opening comment)
            elsif ( /^\s*(\w+)\s*,?\s*\/\*\s*(.+)\s*$/ )
               {
                  $entry = $1;

                  parse_comment( \$t1, \$t2, \$opt, $2 );
               }
            # entry without assignment (none or preceding comment)
            elsif ( /^\s*(\w+)\s*,?\s*$/ )
               {
                  $entry = $1;
               }
            # preceding comment (complete)
            elsif ( /^\s*\/\*\s*(.+)\s*\*\/\s*$/ )
               {
               }
            # preceding comment (opening)
            elsif ( /^\s*\/\*\s*(.+)\s*$/ )
               {
                  parse_comment( \$t1, \$t2, \$opt, $1 );
               }
            # end of enum
            elsif ( /^\s*\}\s*(\w+)\s*\;\s*$/ )
               {
                  $enum = $1;

                  last;
               }
            # blank line?
            else
               {
               }

            if ($entry ne "")
               {
                  print DIRECTFB_V8_CC "          obj->Set( v8::String::NewSymbol(\"$entry\"), v8::Integer::New($entry) );\n";

                  push (@entries, $entry);
               }
         }

      $types{$enum} = {
         NAME    => $enum,
         KIND    => "enum",
         ENTRIES => @entries
      };

      print TEMPLATES_H "extern v8::Handle<v8::ObjectTemplate> ${enum}_template();\n";
   }

#
# Reads stdin until the end of the enum is reached.
# Writes formatted HTML to "types.cc".
#
sub parse_struct
   {
      local @entries;

      while (<>)
         {
            chomp;

            local $entry;

            # without comment
            if ( /^\s*(const )?\s*([\w ]+)\s+(\**)([\w\d\+\[\]]+)(\s*:\s*\d+)?;\s*$/ )
               {
                  $const = $1;
                  $type = $2;
                  $ptr = $3;
                  $entry = $4.$5;
                  $text = "";
               }
            # complete one line entry
            elsif ( /^\s*(const )?\s*([\w ]+)\s+(\**)([\w\d\+\[\]]+)(\s*:\s*\d+)?;\s*\/\*\s*(.+)\*\/\s*$/ )
               {
                  $const = $1;
                  $type = $2;
                  $ptr = $3;
                  $entry = $4.$5;
                  $text = $6;
               }
            # with comment opening
            elsif ( /^\s*(const )?\s*([\w ]+)\s+(\**)([\w\d\+\[\]]+)(\s*:\s*\d+)?;\s*\/\*\s*(.+)\s*$/ )
               {
                  $const = $1;
                  $type = $2;
                  $ptr = $3;
                  $entry = $4.$5;

                  parse_comment( \$t1, \$t2, \$opt, $6 );

                  $text = $t1.$t2;
               }
            # sub
            elsif ( /^\s*struct\s*\{\s*$/ )
               {
                  while (<>)
                     {
                        chomp;
                        last if /^\s*\}\s*([\w\d\+\[\]]+)\s*\;\s*/;
                     }
               }
            elsif ( /^\s*\}\s*(\w+)\s*\;\s*$/ )
               {
                  $struct = $1;

                  trim( \$struct );

                  $struct_list{$struct} = $headline;
                  $type_list{$struct} = $headline;

                  last;
               }

            trim( \$type );

            if ($entry ne "")
               {
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

      # header
      print TEMPLATES_H "extern v8::Handle<v8::Object> ${struct}_construct( const ${struct} *src );\n";
      print TEMPLATES_H "extern void ${struct}_read( ${struct} *dst, v8::Handle<v8::Value> src );\n";

      # _construct
      print TEMPLATES_CC "\n",
                         "v8::Handle<v8::Object>\n",
                         "${struct}_construct( const ${struct} *src )\n",
                         "{\n",
                         "     v8::Handle<v8::Object> obj = v8::Object::New();\n",
                         "\n";

      if ($gen_structs{$struct})
         {
            foreach $entry (@entries)
               {
                  if ($types{$entry->{TYPE}}->{KIND} eq "struct")
                     {
                        print TEMPLATES_CC "     obj->Set( v8::String::NewSymbol( \"$entry->{NAME}\" ), $entry->{TYPE}_construct( &src->$entry->{NAME} ) );\n";
                     }
                  else
                     {
                        print TEMPLATES_CC "     obj->Set( v8::String::NewSymbol( \"$entry->{NAME}\" ), v8::Integer::New(src->$entry->{NAME}) );\n";
                     }
               }
         }

      print TEMPLATES_CC "\n";
      print TEMPLATES_CC "     return obj;\n";
      print TEMPLATES_CC "}\n";

      # _read
      print TEMPLATES_CC "\n",
                         "void\n",
                         "${struct}_read( ${struct} *dst, v8::Handle<v8::Value> src )\n",
                         "{\n",
                         "     if (src.IsEmpty()) { memset( dst, 0, sizeof(${struct}) ); return; }\n",
                         "\n",
                         "     v8::Handle<v8::Object> obj = src->ToObject();\n",
                         "\n";

      if ($gen_structs{$struct})
         {
            foreach $entry (@entries)
               {
                  if ($types{$entry->{TYPE}}->{KIND} eq "struct")
                     {
                        print TEMPLATES_CC "     $entry->{TYPE}_read( &dst->$entry->{NAME}, obj->Get( v8::String::NewSymbol(\"$entry->{NAME}\") )->ToObject() );\n";
                     }
                  else
                     {
                        print TEMPLATES_CC "     dst->$entry->{NAME} = ($entry->{TYPE}) obj->Get( v8::String::NewSymbol(\"$entry->{NAME}\") )->IntegerValue();\n";
                     }
               }
         }

      print TEMPLATES_CC "}\n";
   }

#
# Reads stdin until the end of the function type is reached.
# Writes formatted HTML to "types.cc".
# Parameters are the return type and function type name.
#
sub parse_func ($$)
   {
      local ($rtype, $name) = @_;

      local @entries;
      local %entries_params;
      local %entries_types;
      local %entries_ptrs;

      trim( \$rtype );
      trim( \$name );

      while (<>)
         {
            chomp;

            local $entry;

            # without comment
            if ( /^\s*(const )?\s*([\w ]+)\s+(\**)([\w\d\+\[\]]+)(\s*:\s*\d+)?,?\s*$/ )
               {
                  $const = $1;
                  $type = $2;
                  $ptr = $3;
                  $entry = $4.$5;
                  $text = "";
               }
            # complete one line entry
            elsif ( /^\s*(const )?\s*([\w ]+)\s+(\**)([\w\d\+\[\]]+)(\s*:\s*\d+)?,?\s*\/\*\s*(.+)\*\/\s*$/ )
               {
                  $const = $1;
                  $type = $2;
                  $ptr = $3;
                  $entry = $4.$5;
                  $text = $6;
               }
            # with comment opening
            elsif ( /^\s*(const )?\s*([\w ]+)\s+(\**)([\w\d\+\[\]]+)(\s*:\s*\d+)?,?\s*\/\*\s*(.+)\s*$/ )
               {
                  $const = $1;
                  $type = $2;
                  $ptr = $3;
                  $entry = $4.$5;

                  parse_comment( \$t1, \$t2, \$opt, $6 );

                  $text = $t1.$t2;
               }
            elsif ( /^\s*\)\;\s*$/ )
               {
                  $func_list{$name} = $headline;
                  $type_list{$name} = $headline;

                  last;
               }

            if ($entry ne "")
               {
                  # TODO: Use structure
                  $entries_types{$entry} = $const . $type;
                  $entries_ptrs{$entry} = $ptr;
                  $entries_params{$entry} = $text;

                  push (@entries, $entry);
               }
         }
   }

#
# Reads stdin until the end of the macro is reached.
# Writes formatted HTML to "types.cc".
# Parameters are the macro name, parameters and value.
#
sub parse_macro ($$$)
   {
      local ($macro, $params, $value) = @_;

      trim( \$macro );
      trim( \$params );
      trim( \$value );

      while (<>)
         {
            chomp;

            last unless /\\$/;
         }
   }

########################################################################################################################
## HTML Files
#

sub cc_create ($$$)
   {
      local ($FILE, $filename, $includes) = @_;

      open( $FILE, ">$filename" )
          or die ("*** Can not open '$filename' for writing:\n*** $!");

      print $FILE "#include \"common.h\"\n",
                  "\n",
                  "$includes\n",
                  "namespace DirectFB {\n",
                  "\n";
   }

sub cc_close ($)
   {
      local ($FILE) = @_;

      print $FILE "\n",
                  "}\n";

      close( $FILE );
   }


########################################################################################################################
## Main Function
#

sub gen_doc ($$) {
   local ($project, $version) = @_;

   trim( \$project );
   trim( \$version );

   cc_create( DIRECTFB_V8_H, "directfb_v8.h", "#include \"templates.h\"\n" );
   cc_create( DIRECTFB_V8_CC, "directfb_v8.cc", "#include \"directfb_v8.h\"\n" );

   cc_create( TEMPLATES_H, "templates.h", "" );
   cc_create( TEMPLATES_CC, "templates.cc", "#include <string.h>\n\n#include \"templates.h\"\n" );


   print DIRECTFB_V8_H "void                  Initialize( v8::Handle<v8::ObjectTemplate> obj );\n";
   print DIRECTFB_V8_H "v8::Handle<v8::Value> Create( const v8::Arguments& args );\n";

   print DIRECTFB_V8_CC "v8::Handle<v8::Value>\n",
                        "Create( const v8::Arguments& args )\n",
                        "{\n",
                        "     IDirectFB *dfb;\n",
                        "\n",
                        "     V8_DIRECTFB_CALL( DirectFBCreate( &dfb ) );\n",
                        "\n",
                        "     return Construct( IDirectFB_template(), dfb );\n",
                        "}\n",
                        "\n";


   print DIRECTFB_V8_CC "void\n",
                        "Initialize( v8::Handle<v8::ObjectTemplate> obj )\n",
                        "{\n",
                        "     obj->Set( v8::String::NewSymbol(\"DirectFBCreate\"), v8::FunctionTemplate::New(Create));\n";

   while (<>) {
      chomp;
   
      if ( /^\s*DECLARE_INTERFACE\s*\(\s*(\w+)\s\)\s*$/ ) {
         $interface = $1;

         trim( \$interface );

         if (!defined ($types{$interface})) {
            $types{$interface} = {
               NAME    => $interface,
               KIND    => "interface"
            };


            print TEMPLATES_H "extern v8::Handle<v8::ObjectTemplate> ${interface}_template();\n";
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
         parse_macro( $1, $2, $3 );
      }
      elsif ( /^\s*\/\*\s*$/ ) {
         parse_comment( \$headline, \$detailed, \$options, "" );
      }
      else {
         $headline = "";
         $detailed = "";
         %options  = ();
      }
   }

   print DIRECTFB_V8_CC "}\n";

   cc_close( TEMPLATES_CC );
   cc_close( TEMPLATES_H );

   cc_close( DIRECTFB_V8_CC );
   cc_close( DIRECTFB_V8_H );
}

