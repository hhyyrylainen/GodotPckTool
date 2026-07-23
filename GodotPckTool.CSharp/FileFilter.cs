namespace GodotPckTool;

using System.Text.RegularExpressions;

/// <summary>
///   Filtering options to use when adding or extracting files from a PCK file.
/// </summary>
public class FileFilter
{
    private ulong minSizeLimit;
    private ulong maxSizeLimit = ulong.MaxValue;
    private List<Regex> includePatterns = [];
    private List<Regex> excludePatterns = [];
    private List<Regex> overridePatterns = [];

    public bool Include(ContainedFile file)
    {
        if (overridePatterns.Count > 0)
        {
            foreach (var pattern in overridePatterns)
            {
                if (pattern.IsMatch(file.Path))
                {
                    return true;
                }
            }
        }

        if (includePatterns.Count > 0)
        {
            bool matched = false;

            foreach (var pattern in includePatterns)
            {
                if (pattern.IsMatch(file.Path))
                {
                    matched = true;
                    break;
                }
            }

            if (!matched)
                return false;
        }

        if (file.Size < minSizeLimit)
            return false;

        if (file.Size > maxSizeLimit)
            return false;

        if (excludePatterns.Count > 0)
        {
            foreach (var pattern in excludePatterns)
            {
                if (pattern.IsMatch(file.Path))
                    return false;
            }
        }

        return true;
    }

    public void SetSizeMinLimit(ulong size)
    {
        minSizeLimit = size;
    }

    public void SetSizeMaxLimit(ulong size)
    {
        maxSizeLimit = size;
    }

    public void SetIncludeRegexes(IEnumerable<Regex> filters)
    {
        includePatterns = filters.ToList();
    }

    public void SetExcludeRegexes(IEnumerable<Regex> filters)
    {
        excludePatterns = filters.ToList();
    }

    public void SetIncludeOverrideRegexes(IEnumerable<Regex> filters)
    {
        overridePatterns = filters.ToList();
    }
}
