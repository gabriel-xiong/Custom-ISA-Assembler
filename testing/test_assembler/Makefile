# for valid inputs, check expeceted against actual file outputs. for invalid inputs, 
# verify that there were no intermediate/output files created 

CC=gcc
CFLAGS=-Wall -Wextra -g
TARGET=prog3
TESTDIR=tests

all: $(TARGET)

$(TARGET): prog3.c
	$(CC) $(CFLAGS) prog3.c -o $(TARGET)

test: test_valid test_invalid

test_valid: $(TARGET)
	@passed=0; failed=0; \
	for i in 1 2 3 4 5; do \
		in="$(TESTDIR)/valid$${i}.tk"; \
		out_inter="$(TESTDIR)/out$${i}_intermediate.tk"; \
		out_bin="$(TESTDIR)/out$${i}_output.tko"; \
		exp_inter="$(TESTDIR)/expected_$${i}_intermediate.tk"; \
		exp_bin="$(TESTDIR)/expected_$${i}_output.tko"; \
		rm -f "$$out_inter" "$$out_bin"; \
		./$(TARGET) "$$in" "$$out_inter" "$$out_bin" > /dev/null 2>&1; \
		if [ ! -f "$$out_inter" ] || [ ! -f "$$out_bin" ]; then \
			echo "Valid $${i}: FAILED (missing output files)"; \
			failed=$$((failed+1)); \
			continue; \
		fi; \
		if diff -w "$$out_inter" "$$exp_inter" > /dev/null 2>&1 && cmp -s "$$out_bin" "$$exp_bin"; then \
			echo "Valid $${i}: PASSED"; \
			passed=$$((passed+1)); \
		else \
			echo "Valid $${i}: FAILED (output mismatch)"; \
			failed=$$((failed+1)); \
		fi; \
	done; \
	echo "Valid Passed: $$passed, Failed: $$failed"; \
	test $$failed -eq 0

test_invalid: $(TARGET)
	@passed=0; failed=0; \
	for i in 1 2 3 4 5; do \
		in="$(TESTDIR)/invalid$${i}.tk"; \
		out_inter="$(TESTDIR)/bad$${i}_intermediate.tk"; \
		out_bin="$(TESTDIR)/bad$${i}_output.tko"; \
		rm -f "$$out_inter" "$$out_bin"; \
		./$(TARGET) "$$in" "$$out_inter" "$$out_bin" > /dev/null 2>&1; \
		if [ -f "$$out_inter" ] || [ -f "$$out_bin" ]; then \
			echo "Invalid $${i}: FAILED (produced output files)"; \
			failed=$$((failed+1)); \
		else \
			echo "Invalid $${i}: PASSED"; \
			passed=$$((passed+1)); \
		fi; \
	done; \
	echo "Invalid Passed: $$passed, Failed: $$failed"; \
	test $$failed -eq 0

clean:
	rm -f $(TARGET) $(TESTDIR)/*_intermediate.tk $(TESTDIR)/*_output.tko $(TESTDIR)/out*_*.tk $(TESTDIR)/bad*_*.tk
