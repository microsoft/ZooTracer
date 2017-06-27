using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace ZooTracer
{
    /// <summary>
    /// Interaction logic for PropertyEdit.xaml
    /// </summary>
    public partial class PropertyEdit : UserControl
    {



        public string Text
        {
            get { return (string)GetValue(TextProperty); }
            set { SetValue(TextProperty, value); }
        }

        // Using a DependencyProperty as the backing store for Text.  This enables animation, styling, binding, etc...
        public static readonly DependencyProperty TextProperty =
            DependencyProperty.Register("Text", typeof(string), typeof(PropertyEdit), new FrameworkPropertyMetadata("") { BindsTwoWayByDefault = true, DefaultUpdateSourceTrigger = UpdateSourceTrigger.Explicit });


        public Visibility BaseStateVisibility
        {
            get { return (Visibility)GetValue(BaseStateVisibilityProperty); }
            set { SetValue(BaseStateVisibilityProperty, value); }
        }

        // Using a DependencyProperty as the backing store for BaseStateVisibility.  This enables animation, styling, binding, etc...
        public static readonly DependencyProperty BaseStateVisibilityProperty =
            DependencyProperty.Register("BaseStateVisibility", typeof(Visibility), typeof(PropertyEdit), new PropertyMetadata(Visibility.Visible));
        public Visibility EditStateVisibility
        {
            get { return (Visibility)GetValue(EditStateVisibilityProperty); }
            set { SetValue(EditStateVisibilityProperty, value); }
        }

        // Using a DependencyProperty as the backing store for EditStateVisibility.  This enables animation, styling, binding, etc...
        public static readonly DependencyProperty EditStateVisibilityProperty =
            DependencyProperty.Register("EditStateVisibility", typeof(Visibility), typeof(PropertyEdit), new PropertyMetadata(Visibility.Hidden));


        public PropertyEdit()
        {
            InitializeComponent();
        }

        private void EditButton_Click(object sender, RoutedEventArgs e)
        {
            BaseStateVisibility = Visibility.Hidden;
            EditStateVisibility = Visibility.Visible;
            Editor.Focus();
            // cancel all siblings
            foreach (var c in ((Panel)Parent).Children.OfType<PropertyEdit>())
            {
                if (c != this) c.CancelButton_Click(null, null);
            }
        }

        private void CancelButton_Click(object sender, RoutedEventArgs e)
        {
            GetBindingExpression(PropertyEdit.TextProperty).UpdateTarget();
            BaseStateVisibility = Visibility.Visible;
            EditStateVisibility = Visibility.Hidden;
            Selector.Focus();
        }

        private void ApplyButton_Click(object sender, RoutedEventArgs e)
        {
            GetBindingExpression(PropertyEdit.TextProperty).UpdateSource();
            BaseStateVisibility = Visibility.Visible;
            EditStateVisibility = Visibility.Hidden;
            Selector.Focus();
        }

        private string myOverrideValue = null;
        public string OverrideValue { get { return myOverrideValue; } }
        private void ResetButton_Click(object sender, RoutedEventArgs e)
        {
            var b = GetBindingExpression(PropertyEdit.TextProperty);
            if (b != null && b.ResolvedSource != null && b.ResolvedSourcePropertyName != null)
            {
                var pi = b.ResolvedSource.GetType().GetProperty(b.ResolvedSourcePropertyName);
                if (pi != null)
                {
                    var atts = pi.GetCustomAttributes(typeof(System.Configuration.DefaultSettingValueAttribute), false);
                    if (atts.Length > 0)
                        SetCurrentValue(PropertyEdit.TextProperty, myOverrideValue = ((System.Configuration.DefaultSettingValueAttribute)atts[0]).Value);
                }
            }
        }
    }
}
