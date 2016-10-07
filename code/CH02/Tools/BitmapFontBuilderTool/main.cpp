
/** Tool designed to take the binary width files from BitmapFontBuilder
    http://www.lmnopc.com/bitmapfontbuilder/ and convert them into 
    Ogre .fontdef 'glyph' statements.
    Highly inelegant, but it works!
*/

#include <iostream>
#include <fstream>
#include <string>
#include <math.h>
using namespace std;

int main(int argc, char** argv)
{
    int size;
    std::string datName, newName, fontName, imageName, genGlyph;

		cout << "Enter unique font name: ";
		cin >> fontName;
		cout << "Enter font image name: ";
		cin >> imageName;
		cout << "Enter size of texture(Example: 256): ";
    cin >> size;
    cout << "Enter name of file containing binary widths: ";
    cin >> datName;
		cout << "Generate all glyph statements(Not Recommended)(Y/N): ";
		cin >> genGlyph;
    cout << "Enter name of new text file to create: ";
    cin >> newName;

    int charSize = size / 16;
    int halfWidth = charSize / 2;
    FILE *fp = fopen(datName.c_str(), "rb");

    ofstream o(newName.c_str());

		o << fontName << endl;
		o << "{" << "\n\n";
		o << "\ttype\timage" << endl;
		o << "\tsource\t" << imageName << "\n\n\n";

    int posx = 0;
    int posy = 0; 
    int colcount = 0;
    for (int c = 0; c < 256; c++, colcount++)
    {
        if (colcount == 16)
        {
            colcount = 0;
            posx = 0;
            posy += charSize;
        }

		// Little endian only, but this tool isn't available for OSX anyway
        int width = fgetc(fp) + (fgetc(fp) << 8);
        float thisx_start = posx + halfWidth - (width / 2);
        float thisx_end = posx + halfWidth + (width / 2);

        float u1, u2, v1, v2;
        u1 = thisx_start / (float)(size) ;
        u2 = thisx_end / (float)(size);
        v1 = (float)posy / (float)(size);
        v2 = (float)(posy + charSize) / (float)(size);

				if((genGlyph.at(0) == 'N' || genGlyph.at(0) == 'n') && c >= '!' && c <= '~')
				{
					std::string s = " ";
					s.at(0) = c;
					o << "\tglyph " << s << " " << u1 << " " << v1 << " " << u2 << " " << v2 << std::endl;
				}
				
				if((genGlyph.at(0) != 'N' && genGlyph.at(0) != 'n'))
				{
					std::string s = " ";
					s.at(0) = c;
					o << "\tglyph " << s << " " << u1 << " " << v1 << " " << u2 << " " << v2 << std::endl;
				}
				posx += charSize;

    }
		o << endl;
		o << "}" << endl;
    fclose(fp);

    return 0;
}
