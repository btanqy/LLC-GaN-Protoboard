@echo off

for %%F in (*.pdf) do (
    npx pdf-to-png-converter "%%~fF" --output-folder . --viewport-scale 8.0 --concurrency-limit 1
    ren "%%~nF_page_1.png" "%%~nF.png"
    del "%%~fF"
)

pause