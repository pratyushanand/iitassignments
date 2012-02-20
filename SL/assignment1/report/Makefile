all: clean report.pdf

pdf: report.pdf

report.pdf: report.tex
	pdflatex report.tex
clean:
	rm -f *.ps *.dvi *.aux *.toc *.idx *.ind *.ilg *.log *.out *.pdf
