.PHONY: all judge-all clean

judge-all:
	@python3 judge.py

# simple helper for debugging their data
print_data:
	@python3 helper.py answer/mlp0.ans > ans.txt
	@python3 helper.py output/mlp0.out > out.txt
	@git diff --no-index -- ans.txt out.txt

clean:
	@rm -f mlp_original mlp_improved
	@rm -f output/* answer/* logs/* ans.txt out.txt