default: tree html
	
tree: doxytree.html.cfg
	@echo ---    Building docs... 
	cd .. && doxygen docs/doxytree.html.cfg
	@echo ---    Copying images...
	cp -r images doxytree
	rm -rf doxytree/images/CVS
	@echo ---    Applying frameset patch...
	echo `cp -v tree_overlay/* doxytree`
	@echo ---    Pushing docs live...
	mv ../../../htdocs/doxytree ./tmp1	
	mv -fv doxytree ../../../htdocs/
	rm -rf ./tmp1

html: doxygen.html.cfg
	@echo ---    Building docs...
	cd .. && doxygen docs/doxygen.html.cfg
	@echo ---    Copying images...	
	cp -r images doxydocs
	rm -rf doxydocs/images/CVS
	@echo ---    Pushing docs live...
	mv ../../../htdocs/doxydocs ./tmp2
	mv -fv doxydocs ../../../htdocs/
	rm -rf ./tmp2
	
