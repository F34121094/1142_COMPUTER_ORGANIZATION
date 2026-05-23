MAKEFLAGS += --no-print-directory
judge-all: judge-1 judge-2 judge-3

judge-1:
	@echo "\n= = = = = = = = = = 1 = = = = = = = = = ="
	@$(MAKE) -C 1_cachesim judge-all
judge-2:
	@echo "\n= = = = = = = = = = 2 = = = = = = = = = ="
	@$(MAKE) -C 2_transpose judge-all

judge-3:
	@echo "\n= = = = = = = = = = 3 = = = = = = = = = ="
	@$(MAKE) -C 3_mlp judge-all


clean-all: clean-1 clean-2 clean-3

clean-1:
	@$(MAKE) -C 1_cachesim clean

clean-2:
	@$(MAKE) -C 2_transpose clean

clean-3:
	@$(MAKE) -C 3_mlp clean
