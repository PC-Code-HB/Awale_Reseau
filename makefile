# Dossier projet
CLIENT_DIR = Client
SERVEUR_DIR = Serveur
JEU_DIR = Jeu


# File names
CLIENT_EXEC = client
SERVEUR_EXEC = serveur

CLIENT_SRCS = $(wildcard $(CLIENT_DIR)/src/*.c)
CLIENT_OBJS = $(CLIENT_SRCS:$(CLIENT_DIR)/src/%.c=$(CLIENT_DIR)/obj/%.o)

SERVEUR_SRCS = $(wildcard $(SERVEUR_DIR)/src/*.c)
SERVEUR_OBJS = $(SERVEUR_SRCS:$(SERVEUR_DIR)/src/%.c=$(SERVEUR_DIR)/obj/%.o)

JEU_SRCS = $(wildcard $(JEU_DIR)/src/*.c)
JEU_OBJS = $(JEU_SRCS:$(JEU_DIR)/src/%.c=$(JEU_DIR)/obj/%.o)

# Compilateur C
GCC := gcc


# Options de compilation C
OPTI := -O2
DEBUG := -g -DDEBUG -w
COMPILEFLAGS := $(DEBUG)


# Headers à inclure
INCLUDES := -I$(CLIENT_DIR)/include -I$(SERVEUR_DIR)/include -I$(JEU_DIR)/include



# Options pour l'édition des liens
LDFLAGS := -lm -lpthread 


all: $(CLIENT_EXEC) $(SERVEUR_EXEC)

$(CLIENT_EXEC): $(CLIENT_OBJS) $(JEU_OBJS)
	@echo 'Linking object files : $^'
	@echo 'Invoking : $(GCC) linker'
	$(GCC) $^ $(LDFLAGS) -o $@
	@echo 'Finished linking : $@ executable created'
	@echo ' '

$(SERVEUR_EXEC): $(SERVEUR_OBJS) $(JEU_OBJS)
	@echo 'Linking object files : $^'
	@echo 'Invoking : $(GCC) linker'
	$(GCC) $^ $(LDFLAGS) -o $@
	@echo 'Finished linking : $@ executable created'
	@echo ' '

$(CLIENT_DIR)/obj/%.o: $(CLIENT_DIR)/src/%.c | $(CLIENT_DIR)/obj
	@echo 'Building file : $<'
	@echo 'Invoking : $(GCC) compiler'
	$(GCC) $(COMPILEFLAGS) $(INCLUDES) -c $< -o $@
	@echo 'Finished building'
	@echo ' '

$(SERVEUR_DIR)/obj/%.o: $(SERVEUR_DIR)/src/%.c | $(SERVEUR_DIR)/obj
	@echo 'Building file : $<'
	@echo 'Invoking : $(GCC) compiler'
	$(GCC) $(COMPILEFLAGS) $(INCLUDES) -c $< -o $@
	@echo 'Finished building'
	@echo ' '

$(JEU_DIR)/obj/%.o: $(JEU_DIR)/src/%.c | $(JEU_DIR)/obj
	@echo 'Building file : $<'
	@echo 'Invoking : $(GCC) compiler'
	$(GCC) $(COMPILEFLAGS) $(INCLUDES) -c $< -o $@
	@echo 'Finished building'
	@echo ' '


$(CLIENT_DIR)/obj:
	mkdir -p $@
$(SERVEUR_DIR)/obj:
	mkdir -p $@
$(JEU_DIR)/obj:
	mkdir -p $@

clean:
	rm -f -r $(JEU_DIR)/obj $(CLIENT_DIR)/obj $(SERVEUR_DIR)/obj 
	rm -f $(CLIENT_EXEC) $(SERVEUR_EXEC)


#Spécification
$(CLIENT_DIR)/obj/client.o : $(CLIENT_DIR)/include/client.h $(CLIENT_DIR)/include/socket_client.h $(JEU_DIR)/include/joueur.h
$(CLIENT_DIR)/obj/socket_client.o : $(CLIENT_DIR)/include/socket_client.h


$(SERVEUR_DIR)/obj/socket_serveur.o : $(SERVEUR_DIR)/include/socket_serveur.h $(JEU_DIR)/include/joueur.h
$(SERVEUR_DIR)/obj/serveur.o : $(SERVEUR_DIR)/include/serveur.h $(SERVEUR_DIR)/include/socket_serveur.h $(JEU_DIR)/include/joueur.h

$(JEU_DIR)/obj/joueur.o : $(JEU_DIR)/include/joueur.h
$(JEU_DIR)/obj/awale.o : $(JEU_DIR)/include/awale.h
$(JEU_DIR)/obj/partie.o : $(JEU_DIR)/include/partie.h



