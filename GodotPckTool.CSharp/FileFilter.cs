using System.Text.RegularExpressions;

namespace GodotPckTool;

/// <summary>
///   Filtering options to use when adding or extracting files from a PCK file.
/// </summary>
public class FileFilter
{
    private ulong _minSizeLimit;
    private ulong _maxSizeLimit = ulong.MaxValue;
    private List<Regex> _includePatterns = [];
    private List<Regex> _excludePatterns = [];
    private List<Regex> _overridePatterns = [];

    public bool Include(ContainedFile file)
    {
        if (_overridePatterns.Count > 0)
        {
            foreach (var pattern in _overridePatterns)
            {
                if (pattern.IsMatch(file.Path))
                {
                    return true;
                }
            }
        }

        if (_includePatterns.Count > 0)
        {
            bool matched = false;

            foreach (var pattern in _includePatterns)
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

        if (file.Size < _minSizeLimit)
            return false;

        if (file.Size > _maxSizeLimit)
            return false;

        if (_excludePatterns.Count > 0)
        {
            foreach (var pattern in _excludePatterns)
            {
                if (pattern.IsMatch(file.Path))
                    return false;
            }
        }

        return true;
    }

    public void SetSizeMinLimit(ulong size)
    {
        _minSizeLimit = size;
    }

    public void SetSizeMaxLimit(ulong size)
    {
        _maxSizeLimit = size;
    }

    public void SetIncludeRegexes(IEnumerable<Regex> filters)
    {
        _includePatterns = filters.ToList();
    }

    public void SetExcludeRegexes(IEnumerable<Regex> filters)
    {
        _excludePatterns = filters.ToList();
    }

    public void SetIncludeOverrideRegexes(IEnumerable<Regex> filters)
    {
        _overridePatterns = filters.ToList();
    }
}
