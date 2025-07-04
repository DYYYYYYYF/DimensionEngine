import os

def delete_files(folder_path, exclude_files=None, recursive=False):
    """
    删除指定文件夹下的 .dsm、.dmt、.spv 文件，排除指定文件。

    :param folder_path: 要处理的文件夹路径
    :param exclude_files: 要排除的文件名列表（不含路径）
    :param recursive: 是否递归子目录
    """
    if exclude_files is None:
        exclude_files = ['Material.dmt', 'Material.UI.dmt', 'Material.World.dmt', 'Material.Builtin.GBuffer.dmt', 'Material.Builtin.DeferredLighting.dmt']

    extensions = ('.dsm', '.dmt', '.spv')
    walker = os.walk(folder_path) if recursive else [(folder_path, [], os.listdir(folder_path))]
    for dirpath, _, filenames in walker:
        for filename in filenames:
            if filename.endswith(extensions) and filename not in exclude_files:
                file_path = os.path.join(dirpath, filename)
                try:
                    os.remove(file_path)
                    print(f"已删除: {file_path}")
                except Exception as e:
                    print(f"无法删除 {file_path}: {e}")

if __name__ == "__main__":
    target_folder = r"../Assets"  # 设置目标文件夹路径
    delete_files(target_folder, exclude_files=None, recursive=True)  # recursive=True 启用递归
