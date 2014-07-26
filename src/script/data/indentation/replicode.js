/* kate-script
 * name: Replicode
 * author: Martin Sandsmark <martin.sandsmark@kde.org>
 * license: BSD
 * revision: 1
 * kate-version: 3.4
 * indent-languages: Replicode
 * priority: 0
 *
 */

function indent(line, indentWidth, ch)
{
    var lastLevel = document.firstColumn(line-1);

    if (lastLevel != -1 && document.endsWith(line-1, "[]", true)) {
        return lastLevel + 3;
    } else {
        return -1;
    }
}
