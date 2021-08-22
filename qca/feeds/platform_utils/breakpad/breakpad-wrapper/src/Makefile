

SOURCES_breakpad_qcawrapper = breakpad_qcawrapper.cpp
SOURCES_breakpad_reporter = breakpad_reporter.c

OBJECTS_breakpad_qcawrapper = $(SOURCES_breakpad_qcawrapper:.cpp=.o)
OBJECTS_breakpad_reporter = $(SOURCES_breakpad_reporter:.c=.o)

SHAREDOBJECT_breakpad_qcawrapper = libbreakpad_qcawrapper.so
EXEC_breakpad_reporter = breakpad_reporter

all: $(SHAREDOBJECT_breakpad_qcawrapper) $(EXEC_breakpad_reporter)

$(SHAREDOBJECT_breakpad_qcawrapper): $(OBJECTS_breakpad_qcawrapper)
	$(CXX) -o $@ -shared $(LDFLAGS) $(OBJECTS_breakpad_qcawrapper) -Wl,--whole-archive -lbreakpad_client -Wl,--no-whole-archive

$(EXEC_breakpad_reporter): $(OBJECTS_breakpad_reporter)
	$(CC) -o $@ $(LDFLAGS) $(OBJECTS_breakpad_reporter)

OBJECTS_breakpad_qcawrapper:
	$(CXX) -c -std=c++11 $(CXXFLAGS) $(SOURCES_breakpad_qcawrapper)

OBJECTS_breakpad_reporter:
	$(CC) -c $(CFLAGS) $(SOURCES_breakpad_reporter)

clean:
	$(RM) *.o $(SHAREDOBJECT_breakpad_qcawrapper) $(EXEC_breakpad_reporter)
