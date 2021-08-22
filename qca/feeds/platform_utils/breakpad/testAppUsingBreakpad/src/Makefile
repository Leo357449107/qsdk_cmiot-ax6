

OBJ = testAppUsingBreakpad.o

%.o: %.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS)

testAppUsingBreakpad: $(OBJ)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS)

clean:
	$(RM) *.o testAppUsingBreakpad
