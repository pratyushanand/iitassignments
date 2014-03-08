all: clean Thesis.pdf

pdf: Thesis.pdf

Thesis.pdf: Thesis.tex
	pdflatex Thesis
	bibtex Thesis
	pdflatex Thesis.tex
	pdflatex Thesis.tex
clean:
	rm -f *.ps *.dvi *.aux *.toc *.idx *.ind *.ilg *.log *.out *.pdf *.blg *.bbl *.lof *.lot
