README.md: printbang.h
	echo "<!-- This file is autogenerated from printbang.h using make readme -->" > $@
	awk '\
		# Replace {anchor} with an anchored link to printbang.h \
		{gsub("\\{anchor\\}", "$^" "#L" NR);} \
		# Copy triple slash comments \
		/\/\/\// {{for (i=2; i<NF; i++) printf $$i FS; print $$NF}}; \
		# Copy double star comment blocks \
		/\/\*\*/ {blk=1; printf "\n"}; \
		/\*\*\// {blk=0;}; \
		{if (blk) blk++;} \
		{if (blk > 2) {print;}} \
	' $^ >> $@

.PHONY: readme
readme: README.md

clean:
	$(RM) README.md
