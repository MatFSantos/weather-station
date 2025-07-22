def html_to_c_string(input_file, output_file):
    with open(input_file, 'r', encoding='utf-8') as f:
        lines = f.readlines()

    with open(output_file, 'w', encoding='utf-8') as out:
        out.write('const char html_page[] =\n')
        for line in lines:
            escaped = line.replace('\\', '\\\\').replace('"', '\\"').rstrip('\n')
            out.write(f'"{escaped}\\n"\n')
        out.write(';\n')

html_to_c_string('index.html', 'html_page.c')