.PHONY: all clean judge-all judge-%

CXX := g++

TARGET := csim_cpp
SRCS := csim.cc cachesim.cc
HDRS := cachesim.h

COPY_DST := ../2_transpose/$(TARGET)

all: $(TARGET)

$(TARGET): $(SRCS) $(HDRS)
	@$(CXX) -o $@ $(SRCS)
	@cp $(TARGET) $(COPY_DST)

judge-%:
	@python3 judge.py 'input/$*_*.in'

judge-all:
	@python3 judge.py 'input/*.in'

clean:
	@rm -f $(TARGET) .csim_results
	@rm -f output/* logs/* *.txt