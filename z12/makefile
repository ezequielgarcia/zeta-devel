
ECHO = @echo
CC_PLUS = g++
STRIP = strip

#####################
## IMPROVE ECHOING ##
#####################
 
ifeq ("$(origin V)", "command line")
  BUILD_VERBOSE = $(V)
endif
ifndef BUILD_VERBOSE
  BUILD_VERBOSE = 0
endif
 
ifeq ($(BUILD_VERBOSE),1)
  Q_CC_PLUS =   $(CC_PLUS)
  Q_CC_LINK =   $(CC_PLUS)     
  Q_STRIP =     $(STRIP)
  Q =
else
  Q_CC_PLUS =   $(ECHO) "   Compiling ... "     $@ ; $(CC_PLUS)
  Q_CC_LINK =   $(ECHO) "   Linking ... "       $@ ; $(CC_PLUS)
  Q_STRIP =     $(ECHO) "   Stripping ... "     $@ ; $(STRIP)
  Q = @
endif
 
OUTPUT_NAME = serial
 
##################
## Project Dirs ##
##################
 
OBJ_DIR = obj/
SRC_DIR = src/
 
##############
## Includes ##
##############

CFLAGS= -O2 -g
LIBS = -lpthread -lrt
 
SOURCES_DIRS = .
 
# Obtenemos las ubicaciones de los fuentes y de los objetos
SOURCES  := $(wildcard $(SRC_DIR)*.cc)
OBJECTS  := $(SOURCES:$(SRC_DIR)%.cc=$(OBJ_DIR)%.o)
DEPFILES := $(SOURCES:$(SRC_DIR)%.cc=$(OBJ_DIR)%.P)
 
###########
## RULES ##
###########
 
default: dirs $(OUTPUT_NAME)
 
## LINK RULE (Aca se linkea)
## -----------
 
$(OUTPUT_NAME): $(OBJECTS)
	$(Q_CC_LINK) $^ -Wl,--start-group $(LIBS) -Wl,--end-group -o $@ 
##	$(Q_STRIP) -s $@
 
## OBJECTS RULES (Aca se compila)
## -------------
 
$(OBJ_DIR)%.o: $(SRC_DIR)%.cc
	$(Q_CC_PLUS) -MMD $(CFLAGS) -c $< -o $@
	@cp $(OBJ_DIR)$*.d $(OBJ_DIR)$*.P; \
    	sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
		-e '/^$$/ d' -e 's/$$/ :/' < $(OBJ_DIR)$*.d >> $(OBJ_DIR)$*.P; \
		rm -f $(OBJ_DIR)$*.d
 
.PHONY: clean
clean:
	@$(ECHO) "   Cleaning ..."
	$(Q)rm -rf $(OBJ_DIR)* $(OUTPUT_NAME)
 
.PHONY: dirs
dirs:
	@mkdir -p $(dir $(OBJECTS))
 
# Incluir las reglas que nos dan las dependencias
-include $(DEPFILES)
