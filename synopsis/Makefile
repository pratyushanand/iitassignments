all: clean synopsis.pdf

pdf: synopsis.pdf

synopsis.pdf: synopsis.tex
	pdflatex synopsis
	bibtex synopsis
	pdflatex synopsis.tex
	pdflatex synopsis.tex
clean:
	rm -f *.ps *.dvi *.aux *.toc *.idx *.ind *.ilg *.log *.out *.pdf *.blg *.bbl *.lof *.lot
